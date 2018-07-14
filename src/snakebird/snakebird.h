// -*- mode: c++ -*-

#ifndef GAME_H
#define GAME_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <third-party/cityhash/city.h>
#include <unordered_map>

#include "bit-packer.h"
#include "util.h"

enum Direction {
    UP, RIGHT, DOWN, LEFT,
};

template<int H_, int W_, int FruitCount_,
         int SnakeCount_, int SnakeMaxLen_,
         int GadgetCount_=0, int TeleporterCount_=0>
struct Setup {
    static const int H = H_;
    static const int W = W_;
    static const int FruitCount = FruitCount_;
    static const int SnakeCount = SnakeCount_;
    static const int SnakeMaxLen = SnakeMaxLen_;
    static const int GadgetCount = GadgetCount_;
    static const int TeleporterCount = TeleporterCount_;
    static const int ObjCount = SnakeCount + GadgetCount;

    static const int MapSize = Setup::H * Setup::W;

    // Number of bits used to pack a direction.
    static const int kDirBits = 2;
    static const uint64_t kDirMask = mask_n_bits(kDirBits);
    static const int kIndexBits = integer_length<MapSize>::value;
    static const int kLenBits = integer_length<SnakeMaxLen>::value;

    // Converting between directions and linear coordinate deltas.
    static int apply_direction(Direction dir) {
        static int deltas[] = { -W, 1, W, -1 };
        return deltas[dir];
    }

    static int apply_direction(int dir) {
        return apply_direction(Direction(dir));
    }
};

// The dynamic representation of a Snakebird.
//
// H, W: The height and width of the map
// MaxLen: The maximum number of segments any Snake could have
//   in this scenario.
template<class Setup>
class Snake {
public:
    static const int kTailBits = ((Setup::SnakeMaxLen - 1) * Setup::kDirBits);

    Snake() : tail_(0), len_(0) {
        i_[0] = 0;
    }

    Snake(int i)
        : tail_(0),
          len_(1) {
        assert(i < Setup::MapSize);
        i_[0] = i;
    }

    void grow(Direction dir) {
        std::copy(&i_[0], &i_[len_], &i_[1]);
        i_[0] = i_[1] + Setup::apply_direction(dir);
        ++len_;
        tail_ = (tail_ << Setup::kDirBits) | dir;
    }

    void move(Direction dir) {
        std::copy_backward(&i_[0], &i_[len_ - 1], &i_[len_]);
        i_[0] = i_[1] + Setup::apply_direction(dir);
        tail_ &= ~(Setup::kDirMask << ((len_ - 2) * Setup::kDirBits));
        tail_ = (tail_ << Setup::kDirBits) | dir;
    }

    Direction tail(int i) const {
        return Direction((tail_ >> (i * Setup::kDirBits)) & Setup::kDirMask);
    }

    bool operator<(const Snake& other) const {
        // Doesn't matter which position gets compared.
        if (i_[0] != other.i_[0]) {
            return i_[0] < other.i_[0];
        }
        if (len_ != other.len_) {
            return len_ < other.len_;
        }
        if (tail_ != other.tail_) {
            return tail_ < other.tail_;
        }
        return false;
    }

    template<class P>
    void unpack(const P* packer, typename P::Context* pc) {
        packer->extract(tail_, kTailBits, pc);
        packer->extract(i_[0], Setup::kIndexBits, pc);
        packer->extract(len_, Setup::kLenBits, pc);
        init_locations_from_tail();
    }

    template<class P>
    void pack(P* packer, typename P::Context* pc) const {
        packer->deposit(tail_, kTailBits, pc);
        packer->deposit(i_[0], Setup::kIndexBits, pc);
        packer->deposit(len_, Setup::kLenBits, pc);
    }

    static constexpr uint64_t packed_width() {
        return kTailBits + Setup::kIndexBits + Setup::kLenBits;
    }

    void init_locations_from_tail() {
        for (int i = 1; i < len_; ++i) {
            i_[i] = i_[i - 1] - Setup::apply_direction(tail(i - 1));
        }
    }

    void translate(int32_t delta) {
        for (int i = 0; i < len_; ++i) {
            i_[i] += delta;
        }
    }

    uint64_t tail_;
    int32_t i_[Setup::SnakeMaxLen];
    int32_t len_;
};

class Gadget {
public:
    Gadget() : size_(0) {
    }

    void add(int i) {
        i_[size_++] = i;
    }

    bool operator<(const Gadget& other) const {
        if (size_ != other.size_) {
            return size_ < other.size_;
        }

        for (int i = 0; i < size_; ++i) {
            if (i_[i] != other.i_[i]) {
                return i_[i] < other.i_[i];
            }
        }

        // Note: it's important for the State canonicalization
        // that initial_offset_ does not affect the sorting.
        return false;
    }

    uint16_t initial_offset_ = 0;
    uint16_t size_;
    uint16_t i_[8];
};

struct GadgetState {
    uint16_t template_ ;
    uint16_t offset_ = 0;
};

template<class Setup>
class Map {
public:
    using Snake = typename ::Snake<Setup>;
    using Teleporter = typename std::pair<int, int>;

    explicit Map(const char* base_map) : exit_(0) {
        assert(strlen(base_map) == Setup::MapSize);
        base_map_ = new uint8_t[Setup::MapSize];
        int fruit_count = 0;
        int snake_count = 0;
        int teleporter_count = 0;
        int max_len = 0;
        std::unordered_map<int, int> half_teleporter;
        for (int i = 0; i < Setup::MapSize; ++i) {
            const char c = base_map[i];
            if (c == 'O') {
                if (Setup::FruitCount) {
                    fruit_[fruit_count++] = i;
                }
                base_map_[i] = ' ';
            } else if (c == '*') {
                assert(!exit_);
                base_map_[i] = ' ';
                exit_ = i;
            } else if (c == 'T') {
                if (half_teleporter[c]) {
                    teleporters_[teleporter_count++] =
                        std::make_pair(half_teleporter[c], i);
                } else {
                    half_teleporter[c] = i;
                }
                base_map_[i] = ' ';
            } else if (c == 'R' || c == 'G' || c == 'B') {
                base_map_[i] = ' ';
                Snake snake = Snake(i);
                int len = 0;
                snake.tail_ = trace_tail(base_map, i, &len);
                snake.len_ += len;
                snake.init_locations_from_tail();
                snakes_[snake_count++] = snake;
                max_len = std::max(max_len, (int) snake.len_);
            } else if (isdigit(c)) {
                base_map_[i] = ' ';
                uint32_t index = c - '0';
                assert(index < Setup::GadgetCount);
                if (!gadgets_[index].size_) {
                    gadgets_[index].initial_offset_ = i;
                }
                gadgets_[index].add(i - gadgets_[index].initial_offset_);
            } else if (c == '>' || c == '<' || c == '^' || c == 'v') {
                base_map_[i] = ' ';
            } else {
                base_map_[i] = c;
            }
        }

        std::sort(&gadgets_[0], &gadgets_[Setup::GadgetCount]);

        if (Setup::SnakeMaxLen < max_len + Setup::FruitCount) {
            fprintf(stderr, "Expected SnakeMaxLen >= %d, got %d\n",
                    max_len + Setup::FruitCount,
                    Setup::SnakeMaxLen);
        }
        assert(fruit_count == Setup::FruitCount);
        assert(snake_count == Setup::SnakeCount);
        assert(teleporter_count == Setup::TeleporterCount);
        assert(exit_);
    }

    uint32_t trace_tail(const char* base_map, int i, int* len) const {
        if (base_map[i - 1] == '>') {
            ++*len;
            return RIGHT |
                (trace_tail(base_map, i - 1, len) << Setup::kDirBits);
        }
        if (base_map[i + 1] == '<') {
            ++*len;
            return LEFT |
                (trace_tail(base_map, i + 1, len) << Setup::kDirBits);
        }
        if (base_map[i - Setup::W] == 'v') {
            ++*len;
            return DOWN |
                (trace_tail(base_map, i - Setup::W, len) << Setup::kDirBits);
        }
        if (base_map[i + Setup::W] == '^') {
            ++*len;
            return UP |
                (trace_tail(base_map, i + Setup::W, len) << Setup::kDirBits);
        }

        return 0;
    }

    uint8_t operator[](int i) const {
        return this->base_map_[i];
    }

    uint8_t* base_map_;
    int exit_;
    int fruit_[Setup::FruitCount];
    Snake snakes_[Setup::SnakeCount];
    Gadget gadgets_[Setup::GadgetCount];
    Teleporter teleporters_[Setup::TeleporterCount];
};

template<class Setup>
class State {
public:
    using Snake = typename ::Snake<Setup>;
    using Teleporter = typename std::pair<int, int>;
    using Map = typename ::Map<Setup>;

    static const uint16_t kGadgetDeleted = 0;

    template<bool draw_tail=false>
    class ObjMap {
    public:
        ObjMap(const State& st, const Map& map) {
            draw_objs(st, map, draw_tail);
        }

        bool no_object_at(int i) const {
            return obj_map_[i] == empty_id();
        }

        int fruit_id() const { return 1 + Setup::ObjCount; }
        bool fruit_at(int i) const {
            return obj_map_[i] == fruit_id();
        }

        bool foreign_object_at(int i, int id) const {
            return !no_object_at(i) &&
                obj_map_[i] != id;
        }

        int id_at(int i) const {
            return obj_map_[i];
        }
        int mask_at(int i) const {
            if (id_at(i)) {
                return 1 << (id_at(i) - 1);
            }
            return 0;
        }

    private:
        void draw_objs(const State& st,
                       const Map& map,
                       bool draw_path) {
            memset(obj_map_, empty_id(), Setup::MapSize);
            for (int si = 0; si < Setup::SnakeCount; ++si) {
                draw_snake(st, si, draw_path);
            }
            for (int fi = 0; fi < Setup::FruitCount; ++fi) {
                if (st.fruit_active(fi)) {
                    obj_map_[map.fruit_[fi]] = fruit_id();
                }
            }
            for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
                int offset = st.gadgets_[gi].offset_;
                if (offset != kGadgetDeleted) {
                    const auto& gadget = map.gadgets_[gi];
                    for (int j = 0; j < gadget.size_; ++j) {
                        obj_map_[offset + gadget.i_[j]] = gadget_id(gi);
                    }
                }
            }
        }

        void draw_snake(const State& st, int si, bool draw_path) {
            const Snake& snake = st.snakes_[si];
            int id = snake_id(si);
            if (draw_path) {
                int i = snake.i_[0];
                uint64_t tail = snake.tail_;
                int segment = 0;
                for (int j = 0; j < snake.len_; ++j) {
                    if (j == 0 || !draw_path) {
                        obj_map_[i] = id;
                    } else {
                        switch (segment) {
                        case UP: obj_map_[i] = '^'; break;
                        case DOWN: obj_map_[i] = 'v'; break;
                        case LEFT: obj_map_[i] = '<'; break;
                        case RIGHT: obj_map_[i] = '>'; break;
                        }
                    }
                    segment = tail & Setup::kDirMask;
                    i -= Setup::apply_direction(tail & Setup::kDirMask);
                    tail >>= Setup::kDirBits;
                }
            } else {
                for (int i = 0; i < snake.len_; ++i) {
                    obj_map_[snake.i_[i]] = id;
                }
            }
        }

        uint8_t obj_map_[Setup::MapSize];
    };
    static int empty_id() { return 0; }
    static int snake_id(int si) { return (1 + si); }
    static int snake_mask(int si) { return 1 << si; }
    static int gadget_id(int i) { return (1 + i + Setup::SnakeCount); }
    static int gadget_mask(int i) { return 1 << (Setup::SnakeCount + i); }

    State() {
        fruit_ = mask_n_bits(Setup::FruitCount);
        for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
            gadgets_[gi].template_ = gi;
        }
    }

    State(const Map& map) : State() {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            snakes_[si] = map.snakes_[si];
        }
        for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
            gadgets_[gi].offset_ = map.gadgets_[gi].initial_offset_;
            gadgets_[gi].template_ = gi;
        }
    }

    void print(const Map& map) const {
        ObjMap<true> obj_map(*this, map);

        for (int i = 0; i < Setup::H; ++i) {
            for (int j = 0; j < Setup::W; ++j) {
                int l = i * Setup::W + j;
                bool teleport = false;
                for (auto t : map.teleporters_) {
                    if (t.first == l || t.second == l) {
                        teleport = true;
                    }
                }
                if (!obj_map.no_object_at(l)) {
                    int id = obj_map.id_at(l);
                    char c;
                    if (id < (Setup::SnakeCount + 1)) {
                        c = 'A' + (id - 1);
                    } else if (id < Setup::ObjCount + 1) {
                        c = '0' + (id - 1 - Setup::SnakeCount);
                    } else if (id == obj_map.fruit_id()) {
                        c = 'Q';
                    } else {
                        c = id;
                    }
                    printf("%c", c);
                } else if (l == map.exit_) {
                    printf("*");
                } else if (teleport) {
                    printf("X");
                } else {
                    printf("%c", map[l]);
                }
            }
            printf("\n");
        }
        printf("\n");
    }

    void delete_fruit(int i) {
        fruit_ = fruit_ & ~(1 << i);
    }

    bool fruit_active(int i) const {
        return (fruit_ & (1 << i)) != 0;
    }

    bool do_valid_moves(const Map& map,
                        std::function<bool(State)> fun) const {
        static Direction dirs[] = {
            UP, RIGHT, DOWN, LEFT,
        };
        ObjMap<> obj_map(*this, map);
        uint32_t tele_mask = teleporter_overlap(map, obj_map);
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            if (!snakes_[si].len_) {
                continue;
            }
            // There has to be a cleaner way to do this...
            State push_st(*this);
            push_st.snakes_[si].len_--;
            ObjMap<> push_map(push_st, map);
            for (auto dir : dirs) {
                int delta = Setup::apply_direction(dir);
                int to = snakes_[si].i_[0] + delta;
                int pushed_ids = 0;
                int fruit_index = 0;
                if (is_valid_grow(map, to, &fruit_index)) {
                    State new_state(*this);
                    new_state.snakes_[si].grow(dir);
                    new_state.delete_fruit(fruit_index);
                    if (new_state.process_gravity(map, tele_mask)) {
                        if (fun(new_state)) {
                            return true;
                        }
                    }
                } if (is_valid_move(map, obj_map, to)) {
                    State new_state(*this);
                    new_state.snakes_[si].move(dir);
                    if (new_state.process_gravity(map, tele_mask)) {
                        if (fun(new_state)) {
                            return true;
                        }
                    }
                } else if (is_valid_push(map, push_map,
                                         snake_id(si),
                                         snakes_[si].i_[0],
                                         delta,
                                         &pushed_ids) &&
                           !(pushed_ids & snake_mask(si))) {
                    State new_state(*this);
                    new_state.snakes_[si].move(dir);
                    new_state.do_pushes(obj_map, pushed_ids, delta);
                    if (new_state.process_gravity(map, tele_mask)) {
                        if (fun(new_state)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    void canonicalize(const Map& map) {
        std::sort(&snakes_[0], &snakes_[Setup::SnakeCount]);
        if (Setup::GadgetCount > 0) {
            std::sort(&gadgets_[0], &gadgets_[Setup::GadgetCount],
                      [&map] (const GadgetState& a, const GadgetState& b) {
                          const Gadget& ag = map.gadgets_[a.template_];
                          const Gadget& bg = map.gadgets_[b.template_];
                          if (ag < bg) {
                              return true;
                          }
                          if (bg < ag) {
                              return false;
                          }
                          return a.offset_ < b.offset_;
                      });
        }
    }

    uint32_t teleporter_overlap(const Map& map, const ObjMap<>& objmap) const {
        uint32_t mask = 0;
        const uint32_t width = Setup::ObjCount;
        for (int ti = 0; ti < Setup::TeleporterCount; ++ti) {
            mask |=
                ((objmap.mask_at(map.teleporters_[ti].first)) |
                 (objmap.mask_at(map.teleporters_[ti].second) << width))
                << (width * 2 * ti);
        }
        return mask;
    }

    void do_pushes(const ObjMap<>& obj_map, int pushed_ids, int push_delta) {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            if (pushed_ids & snake_mask(si)) {
                snakes_[si].translate(push_delta);
            }
        }
        for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
            if (pushed_ids & gadget_mask(gi)) {
                gadgets_[gi].offset_ += push_delta;
            }
        }
    }

    bool destroy_if_intersects_hazard(const Map& map,
                                      const ObjMap<>& obj_map,
                                      int pushed_ids) {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            if (pushed_ids & snake_mask(si)) {
                if (snake_intersects_hazard(map, snakes_[si]))
                    return true;
            }
        }
        for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
            if (pushed_ids & gadget_mask(gi)) {
                if (gadget_intersects_hazard(map, gi)) {
                    gadgets_[gi].offset_ = kGadgetDeleted;
                }
            }
        }
        return false;
    }

    bool is_valid_grow(const Map& map,
                       int to,
                       int* fruit_index) const {
        for (int fi = 0; fi < Setup::FruitCount; ++fi) {
            int fruit = map.fruit_[fi];
            if (fruit_active(fi) &&
                fruit == to) {
                *fruit_index = fi;
                return true;
            }
        }

        return false;
    }

    bool is_valid_move(const Map& map,
                       const ObjMap<>& obj_map,
                       int to) const {
        if (obj_map.no_object_at(to) && empty_terrain_at(map, to)) {
            return true;
        }

        return false;
    }

    bool empty_terrain_at(const Map& map, int i) const {
        return map[i] == ' ';
    }

    bool is_valid_push(const Map& map,
                       const ObjMap<>& obj_map,
                       int pusher_id,
                       int push_at,
                       int delta,
                       int* pushed_ids) const __attribute__((noinline)) {
        int to = push_at + delta;

        if (obj_map.no_object_at(to) ||
            obj_map.id_at(to) == pusher_id ||
            obj_map.fruit_at(to)) {
            return false;
        }

        *pushed_ids = obj_map.mask_at(to);
        bool again = true;

        while (again) {
            again = false;
            for (int si = 0; si < Setup::SnakeCount; ++si) {
                if (*pushed_ids & snake_mask(si)) {
                    int new_pushed_ids = 0;
                    if (!snake_can_be_pushed(map, obj_map,
                                             si,
                                             delta,
                                             &new_pushed_ids)) {
                        return false;
                    }
                    if (new_pushed_ids & ~(*pushed_ids)) {
                        *pushed_ids |= new_pushed_ids;
                        again = true;
                    }
                }
            }
            for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
                if (*pushed_ids & gadget_mask(gi)) {
                    int new_pushed_ids = 0;
                    if (!gadget_can_be_pushed(map,
                                              obj_map,
                                              gi,
                                              delta,
                                              &new_pushed_ids)) {
                        return false;
                    }
                    if (new_pushed_ids & ~(*pushed_ids)) {
                        *pushed_ids |= new_pushed_ids;
                        again = true;
                    }
                }
            }
        }

        return true;
    }

    bool snake_can_be_pushed(const Map& map,
                             const ObjMap<>& obj_map,
                             int si,
                             int delta,
                             int* pushed_ids) const __attribute__((noinline)) {
        const Snake& snake = snakes_[si];

        for (int i = 0; i < snake.len_; ++i) {
            // The space the Snake's head would be pushed to.
            int to = snake.i_[i] + delta;
            if (!empty_terrain_at(map, to)) {
                return false;
            }
            if (obj_map.fruit_at(to)) {
                return false;
            }
            if (obj_map.foreign_object_at(to, snake_id(si))) {
                *pushed_ids |= obj_map.mask_at(to);
            }
        }

        return true;
    }

    bool gadget_can_be_pushed(const Map& map,
                              const ObjMap<>& obj_map,
                              int gi,
                              int delta,
                              int* pushed_ids) const __attribute__((noinline)) {
        const auto& gadget = map.gadgets_[gi];
        int offset = gadgets_[gi].offset_;

        for (int j = 0; j < gadget.size_; ++j) {
            int i = gadget.i_[j] + offset + delta;
            if (!empty_terrain_at(map, i)) {
                return false;
            }
            if (obj_map.fruit_at(i)) {
                return false;
            }
            if (!obj_map.no_object_at(i)) {
                *pushed_ids |= obj_map.mask_at(i);
            }
        }

        return true;
    }

    bool process_teleports(const Map& map, const ObjMap<>& obj_map,
                           uint32_t orig_tele_mask,
                           uint32_t new_tele_mask) {
        uint32_t only_new = new_tele_mask & ~orig_tele_mask;
        int test = 1;
        bool teleported = false;
        // This is over-engineered for the possibility of multiple
        // teleporters. But those don't actually appear in the game,
        // and there are some interesting semantic problems with them
        // with two different teleporter pairs being triggered at the
        // same time, so it's just a guess that this is how they'd
        // work.
        for (int ti = 0; ti < Setup::TeleporterCount; ++ti) {
            int delta = map.teleporters_[ti].second -
                map.teleporters_[ti].first;
            for (int dir = 0; dir < 2; ++dir) {
                for (int si = 0; si < Setup::SnakeCount; ++si) {
                    if (test & only_new) {
                        if (try_snake_teleport(map, obj_map, si, delta)) {
                            teleported = true;
                        }
                    }
                    test <<= 1;
                }
                for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
                    if (test & only_new) {
                        if (try_gadget_teleport(map, obj_map, gi, delta)) {
                            teleported = true;
                        }
                    }
                    test <<= 1;
                }
                // Delta was from A to B, just negate it for
                // handling the B to A case.
                delta = -delta;
            }
        }

        return teleported;
    }

    bool try_snake_teleport(const Map& map,
                            const ObjMap<>& obj_map,
                            int si, int delta) {
        const Snake& snake = snakes_[si];

        for (int i = 0; i < snake.len_; ++i) {
            int to = snake.i_[i] + delta;
            if (map[to] != ' ') {
                return false;
            }
            // If segment X of a snake would teleport into the space
            // occupied by segment Y of the same snake pre-teleport,
            // is the teleport blocked? I'm assuming yes. If not,
            // this should be a foreign_object_at instead.
            if (!obj_map.no_object_at(to)) {
                return false;
            }
        }

        snakes_[si].translate(delta);

        return true;
    }

    bool try_gadget_teleport(const Map& map,
                             const ObjMap<>& obj_map,
                             int gi,
                             int delta) {
        const auto& gadget = map.gadgets_[gi];
        int offset = gadgets_[gi].offset_ + delta;

        for (int j = 0; j < gadget.size_; ++j) {
            int to = gadget.i_[j] + offset;
            if (map[to] != ' ') {
                return false;
            }
            if (!obj_map.no_object_at(to)) {
                return false;
            }
        }

        // There's a funny thing here where a sparse gadget could
        // theoretically teleport halfway over a map edge, since
        // the solid border tile protection doesn't work there.
        // But if a solution ended up abusing that, it'd be easy to
        // fix by just adding more padding to the map.

        gadgets_[gi].offset_ += delta;
        return true;
    }

    bool process_gravity(const Map& map, uint32_t orig_tele_mask)
        __attribute__((noinline)) {
        uint32_t recompute_falling = mask_n_bits(Setup::ObjCount);
        uint32_t falling[Setup::ObjCount];

    again:
        // FIXME. Figure out if exits and teleporters have different
        // priority. Is it possible to construct a case where that
        // matters?
        check_exits(map);
        // FIXME: The teleporter + gravity interaction doesn't quite
        // match the actual game. There you can have the scenario where
        // snake A supports snake B. Then:
        // 1. A moves, and goes through a teleporter
        // 2. Both A and B drop due to gravity
        // 3. B is now on a teleporter. The remote side is not blocked
        //    by A. But B does not teleport.
        // Constructing more exact test cases is proving tricky.
        ObjMap<> obj_map(*this, map);
        uint32_t new_tele_mask = teleporter_overlap(map, obj_map);
        if (new_tele_mask & ~orig_tele_mask) {
            if (process_teleports(map, obj_map, orig_tele_mask,
                                  new_tele_mask)) {
                orig_tele_mask = teleporter_overlap(map,
                                                    ObjMap<>(*this, map));
                goto again;
            }
        }
        orig_tele_mask = new_tele_mask;

        uint32_t supported = 0;
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            int mask = 0;
            if (snakes_[si].len_) {
                if (recompute_falling &
                    snake_mask(si)) {
                    mask = is_snake_falling(map, obj_map, si);
                }
            }
            falling[si] = mask;
            if (!mask) {
                supported |= snake_mask(si);
            }
        }
        for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
            int offset = gadgets_[gi].offset_;
            int mask = 0;
            if (offset != kGadgetDeleted) {
                if (recompute_falling &
                    gadget_mask(gi)) {
                    mask = is_gadget_falling(map, obj_map, gi);
                }
            }
            falling[Setup::SnakeCount + gi] = mask;
            if (!mask) {
                supported |= gadget_mask(gi);
            }
        }
        recompute_falling = 0;

        while (1) {
            bool again = false;
            for (int i = 0; i < Setup::ObjCount; ++i) {
                int mask = 1 << i;
                if (!(supported & mask) &&
                    (supported & falling[i])) {
                    supported |= mask;
                    again = true;
                }
            }
            if (!again) {
                break;
            }
        }

        uint32_t to_push = mask_n_bits(Setup::ObjCount)
            & ~supported;

        if (to_push) {
            do_pushes(obj_map, to_push, Setup::W);
            if (destroy_if_intersects_hazard(map, obj_map, to_push)) {
                return false;
            }
            recompute_falling |= to_push;
            goto again;
        }

        return true;
    }

    void check_exits(const Map& map) {
        if (fruit_) {
            // Can't use exits until all fruit are eaten.
            return;
        }

        for (auto& snake : snakes_) {
            if (snake.len_) {
                if (snake_head_at_exit(map, snake)) {
                    snake.len_ = 0;
                    snake.i_[0] = 0;
                    snake.tail_ = 0;
                }
            }
        }
    }

    bool win() {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            if (snakes_[si].len_) {
                return false;
            }
        }

        return true;
    }

    int is_snake_falling(const Map& map,
                         const ObjMap<>& obj_map,
                         int si) const {
        const Snake& snake = snakes_[si];
        int pushed_ids = snake_mask(si);

        for (int i = 0; i < snake.len_; ++i) {
            int below = snake.i_[i] + Setup::W;
            if (map[below] == '.' ||
                obj_map.fruit_at(below)) {
                return 0;
            }
            if (obj_map.foreign_object_at(below, snake_id(si))) {
                pushed_ids |= obj_map.mask_at(below);
            }
        }

        return pushed_ids;
    }

    int is_gadget_falling(const Map& map,
                          const ObjMap<>& obj_map,
                          int gi) const {
        const auto& gadget = map.gadgets_[gi];

        int pushed_ids = gadget_mask(gi);
        int id = gadget_id(gi);

        for (int j = 0; j < gadget.size_; ++j) {
            int at = gadget.i_[j] + gadgets_[gi].offset_;
            int below = at + Setup::W;
            if (map[below] == '.' ||
                map[below] == '#' ||
                obj_map.fruit_at(below)) {
                return 0;
            }
            if (obj_map.foreign_object_at(below, id)) {
                pushed_ids |= obj_map.mask_at(below);
            }
        }

        return pushed_ids;
    }

    bool snake_head_at_exit(const Map& map, const Snake& snake) const {
        // Only the head of the snake will trigger an exit
        return snake.i_[0] == map.exit_;
    }

    bool snake_intersects_hazard(const Map& map, const Snake& snake) const {
        for (int i = 0; i < snake.len_; ++i) {
            int at = snake.i_[i];
            if (map[at] == '~' || map[at] == '#')
                return true;
        }

        return false;
    }

    bool gadget_intersects_hazard(const Map& map,
                                  int gi) const {
        int offset = gadgets_[gi].offset_;
        if (offset == kGadgetDeleted)
            return false;
        const auto& gadget = map.gadgets_[gi];
        for (int j = 0; j < gadget.size_; ++j) {
            // Spikes aren't a hazard for gadgets.
            if (map[gadget.i_[j] + offset] == '~')
                return true;
        }

        return false;
    }

    static constexpr uint64_t packed_width() {
        return Snake::packed_width() * Setup::SnakeCount +
            Setup::FruitCount +
            Setup::kIndexBits * Setup::GadgetCount;
    }

    struct Packed {
        using P = Packer<packed_width()>;

        Packed() {}
        Packed(const State& st) {
            typename P::Context pc;
            st.pack(&p_, &pc);
            p_.flush(&pc);
        }

        static constexpr int width_bytes() {
            return P::Bytes;
        }

        uint8_t& at(size_t i) { return p_.bytes_[i]; }
        const uint8_t& at(size_t i) const { return p_.bytes_[i]; }

        uint8_t* bytes() { return p_.bytes_; }
        const uint8_t* bytes() const { return p_.bytes_; }

        uint64_t hash() const {
            return CityHash64((char*) bytes(),
                              width_bytes());
        }

        bool operator==(const Packed& other) const {
            return memcmp(bytes(), other.bytes(), P::Bytes) == 0;
        }

        bool operator<(const Packed& other) const {
            // This is morally a memcmp, but it's opencoded since we
            // don't care about what the ordering is, just that there
            // is one. (The difference is due to endian issues).
            //
            // return memcmp(p_.bytes_, other.p_.bytes_, P::Bytes) < 0;
            int i = 0;
            for (; i + 7 < P::Bytes; i += 8) {
                uint64_t a = *((uint64_t*) (bytes() + i));
                uint64_t b = *((uint64_t*) (other.bytes() + i));
                if (a != b)
                    return a < b;
            }
            for (; i + 3 < P::Bytes; i += 4) {
                uint32_t a = *((uint32_t*) (bytes() + i));
                uint32_t b = *((uint32_t*) (other.bytes() + i));
                if (a != b)
                    return a < b;
            }
            for (; i < P::Bytes; ++i) {
                if (at(i) != other.at(i)) {
                    return at(i) < other.at(i);
                }
            }
            return false;
        }

        P p_;
    };

    State(const Packed& p) : State() {
        typename Packed::P::Context pc;
        unpack(&p.p_, &pc);
    };

    template<class P>
    void unpack(const P* packer, typename P::Context* pc) {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            snakes_[si].unpack(packer, pc);
        }
        packer->extract(fruit_, Setup::FruitCount, pc);
        for (int gi = 0 ; gi < Setup::GadgetCount; ++gi) {
            packer->extract(gadgets_[gi].offset_,
                            Setup::kIndexBits,
                            pc);
        }
    }

    template<class P>
    void pack(P* packer, typename P::Context* pc) const {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            snakes_[si].pack(packer, pc);
        }
        packer->deposit(fruit_, Setup::FruitCount, pc);
        for (int gi = 0 ; gi < Setup::GadgetCount; ++gi) {
            packer->deposit(gadgets_[gi].offset_,
                            Setup::kIndexBits,
                            pc);
        }
    }

    Snake snakes_[Setup::SnakeCount];
    GadgetState gadgets_[Setup::GadgetCount];
    uint64_t fruit_;
};

#endif // GAME_H

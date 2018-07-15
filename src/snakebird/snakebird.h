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

// A puzzle scenario description. All other classes are parametrized
// with this.
//
// H: Map height
// W: Map width
// FruitCount: Number of initial fruit on the board.
// SnakeCount: Number of initial Snakebirds on the board.
// SnakeMaxLen: The maximum size that a Snakebird could theoretically
//   grow to. (Normally the maximum initial length + number of fruit).
// GadgetCount: Number of movable objects on the board.
// TeleporterCount: Number of teleporters on the map.
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
// A Snakebird consists of a queue of segments, with each segment
// being orthogonally adjacent to the others. As the Snakebird moves,
// the head of the bird (i.e. segment 0) moves by one space to the
// target space, and all the other segments move to the location
// vacated by the previous segment. If a Snakebird grows, none of
// the existing segments move. Instead a new segment is added to
// the head of the queue.
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

    // Extends the head of the snake in the given direction, without
    // shortening the snake at the tail.
    void grow(Direction dir) {
        std::copy(&i_[0], &i_[len_], &i_[1]);
        i_[0] = i_[1] + Setup::apply_direction(dir);
        ++len_;
        tail_ = (tail_ << Setup::kDirBits) | dir;
    }

    // Moves the head of the snake to the given direction; then
    // moves the second segment of the snake to the space originally
    // occupied by the head, the third segment to the space occupied
    // by the second, etc.
    void move(Direction dir) {
        std::copy_backward(&i_[0], &i_[len_ - 1], &i_[len_]);
        i_[0] = i_[1] + Setup::apply_direction(dir);
        tail_ &= ~(Setup::kDirMask << ((len_ - 2) * Setup::kDirBits));
        tail_ = (tail_ << Setup::kDirBits) | dir;
    }

    // Returns the directions the Snakebird has moved in (i==0 is
    // the most recent move). May not be called with a number larger
    // than len_ - 1.
    Direction tail(int i) const {
        return Direction((tail_ >> (i * Setup::kDirBits)) & Setup::kDirMask);
    }

    // Compares to Snakebirds, based on their location and shape.
    bool operator<(const Snake& other) const {
        // Doesn't matter which segment gets compared, as long as
        // it's consistent.
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

    // Deserializes a Snakebird from bits.
    template<class P>
    void unpack(const P* packer, typename P::Context* pc) {
        packer->extract(tail_, kTailBits, pc);
        packer->extract(i_[0], Setup::kIndexBits, pc);
        packer->extract(len_, Setup::kLenBits, pc);
        init_locations_from_tail();
    }

    // Serializes this Snakebird to bits.
    template<class P>
    void pack(P* packer, typename P::Context* pc) const {
        packer->deposit(tail_, kTailBits, pc);
        packer->deposit(i_[0], Setup::kIndexBits, pc);
        packer->deposit(len_, Setup::kLenBits, pc);
    }

    // The amount of bits needed to represent a Snakebird in
    // this Setup.
    static constexpr uint64_t packed_width() {
        return kTailBits + Setup::kIndexBits + Setup::kLenBits;
    }

    // Resets the location of all other segments of the Snakebird to
    // be consistent with the shape and the location of the head.
    void init_locations_from_tail() {
        for (int i = 1; i < len_; ++i) {
            i_[i] = i_[i - 1] - Setup::apply_direction(tail(i - 1));
        }
    }

    // Moves the Snakebird by delta steps (in the linear coordinate
    // system). All segments of the Snakebird move by the same amount,
    // so the shape does not change.
    void translate(int32_t delta) {
        for (int i = 0; i < len_; ++i) {
            i_[i] += delta;
        }
    }

    // The last len_ - 1 directions given to move() and grow().
    // Describes the shape of the Snakebird, independent of its
    // location. Encoded as two bits per segment (matching the
    // Direction enums). The most recent move is encoded in the
    // least significant bits.
    uint64_t tail_;
    // The locations (in the linear coordinate system) of each of the
    // Snakebird's segments.
    int32_t i_[Setup::SnakeMaxLen];
    // The number of segments the Snakebird consists of. Must be at least 2.
    int32_t len_;
};

// A Gadget is a movable object with a fixed shape. The parts
// of the Gadget do not need to be contiguous.
//
// This class describes the shape and initial location of a Gadget.
// As the Gadget is moved, the movement is tracked as a delta to
// the initial location.
class Gadget {
public:
    // Create a Gadget with no parts.
    Gadget() : size_(0) {
    }

    // Add a new part to the Gadget at the given linear coordinate
    // offset compared to the first part. (I.e. the first part should
    // always have an offset of 0).
    void add(int offset) {
        i_[size_++] = offset;
    }

    // Compare two Gadgets, by shape and location.
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

    // The actual location of the first part of the Gadget in
    // the initial setup.
    uint16_t initial_offset_ = 0;
    // The number of parts in the gadget.
    uint16_t size_;
    // The offset from this part to the first part of the Gadget.
    uint16_t i_[8];
};

// The dynamically changing parts of a movable object.
struct GadgetState {
    // An index to an array of Gadgets, which corresponds to the
    // static setup of this object.
    uint16_t template_ ;
    // The amount (in the linear coordinate system) by which this
    // object has moved after the initial setup.
    uint16_t offset_ = 0;
};

// Any data that is immutable based on the scenario description,
// and never changes between States.
//
// - The locations of solid ground, hazards, fruit, and teleporters.
// - The shapes and initial locations of all movable objects.
//
// Also contains state that needs to be copied to the initial
// state:
//
// - The shape and location of each Snakebird.
template<class Setup>
class Map {
public:
    using Snake = typename ::Snake<Setup>;
    using Teleporter = typename std::pair<int, int>;

    // Constructs a Map object from the given base map description.
    // [O] is a fruit, [*] is an exit, [T] is a teleporter, [RGB] are
    // the heads of Snakebirds; [<>v^] show the shape of the
    // Snakebird; [0-9] are the parts of different gadgets, [.] is
    // solid ground, [~#] are hazards.
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

// The serialization of a State object into a bitstream of kWidthBits
// bits (rounded up to the next byte).
//
// All the serialization and deserialization happens with Packer; in
// practice this is just convenience functions for operating on an
// opaque byte array.
template<class State, size_t kWidthBits>
struct PackedState {
    using P = Packer<kWidthBits>;

    PackedState() {}
    PackedState(const State& st) {
        typename P::Context pc;
        st.pack(&p_, &pc);
        p_.flush(&pc);
    }

    // The size of the serialized output in bytes.
    static constexpr int width_bytes() { return P::Bytes; }
    // Returns the i'th element in the serialized state.
    uint8_t& at(size_t i) { return p_.bytes_[i]; }
    const uint8_t& at(size_t i) const { return p_.bytes_[i]; }

    // Returns the full serialized state.
    uint8_t* bytes() { return p_.bytes_; }
    const uint8_t* bytes() const { return p_.bytes_; }

    // Computes a hashcode for this state.
    uint64_t hash() const {
        return CityHash64((char*) bytes(),
                          width_bytes());
    }

    // Compares two serialized states.

    bool operator==(const PackedState& other) const {
        return memcmp(bytes(), other.bytes(), P::Bytes) == 0;
    }

    bool operator<(const PackedState& other) const {
        // This is morally a memcmp, but it's opencoded since we
        // don't care about what the ordering is, just that there
        // is one. (The difference is due to endian issues, which
        // is why this kind of optimization can't happen automatically).
        //
        // return memcmp(bytes(), other.bytes(), P::Bytes) < 0;
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

// An index for all the the mutable parts of a State (Fruits,
// Snakebirds, Gadgets), queryable my map coordinate.
//
// If kDrawTail is true, uses [<>v^] to draw the exact shape of
// Snakebirds rather than just marking the locations of the
// segments.
template<class State, bool kDrawTail=false>
class ObjMap {
public:
    using Map = typename State::Map;
    using Setup = typename State::Setup;
    using Snake = typename State::Snake;
    using ObjMask = typename State::ObjMask;

    ObjMap(const State& st, const Map& map) {
        draw_objs(st, map, kDrawTail);
    }

    // Returns true if no objects overlap the given map coordinate.
    bool no_object_at(int i) const {
        return obj_map_[i] == State::empty_id();
    }

    // Returns true if there is an (uneaten) fruit at the given map
    // coordinate.
    bool fruit_at(int i) const {
        return obj_map_[i] == fruit_id();
    }

    // Returns true if the given map coordinate contains an object
    // (different from the given object id).
    bool foreign_object_at(int i, int id) const {
        return !no_object_at(i) &&
            obj_map_[i] != id;
    }

    // Returns the id of the object at the given map coordinate.
    int id_at(int i) const {
        return obj_map_[i];
    }

    // If the given map location contains an object, returns
    // a bit-mask with the bit corresponding to that object set.
    // Otherwise returns 0.
    ObjMask mask_at(int i) const {
        if (id_at(i)) {
            return 1 << (id_at(i) - 1);
        }
        return 0;
    }

    int fruit_id() const { return 1 + Setup::ObjCount; }

private:
    void draw_objs(const State& st,
                   const Map& map,
                   bool draw_path) {
        memset(obj_map_, State::empty_id(), Setup::MapSize);
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
            if (offset != State::kGadgetDeleted) {
                const auto& gadget = map.gadgets_[gi];
                for (int j = 0; j < gadget.size_; ++j) {
                    obj_map_[offset + gadget.i_[j]] = State::gadget_id(gi);
                }
            }
        }
    }

    void draw_snake(const State& st, int si, bool draw_path) {
        const Snake& snake = st.snakes_[si];
        int id = State::snake_id(si);
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

template<class Setup_>
class State {
    using Setup = Setup_;
    using Snake = typename ::Snake<Setup>;
    using Teleporter = typename std::pair<int, int>;

    // The size (in bits) of a serialized state object.
    static constexpr uint64_t packed_bits() {
        return Snake::packed_width() * Setup::SnakeCount +
            Setup::FruitCount +
            Setup::kIndexBits * Setup::GadgetCount;
    }

public:
    using Map = typename ::Map<Setup>;
    using Packed = PackedState<State, packed_bits()>;
    // A bitmask of objects. Bits (0 : Setup::SnakeCount] are
    // the Snakebirds, bits (Setup::SnakeCount : Setup::ObjCount]
    // are Gadgets. Fruit are not tracked in the mask.
    using ObjMask = uint32_t;

    // The null state.
    State() {
        fruit_ = mask_n_bits(Setup::FruitCount);
        for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
            gadgets_[gi].template_ = gi;
        }
    }

    // The initial state.
    State(const Map& map) : State() {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            snakes_[si] = map.snakes_[si];
        }
        for (int gi = 0; gi < Setup::GadgetCount; ++gi) {
            gadgets_[gi].offset_ = map.gadgets_[gi].initial_offset_;
            gadgets_[gi].template_ = gi;
        }
    }

    // De-serializes a state from bytes.
    State(const Packed& p) : State() {
        typename Packed::P::Context pc;
        unpack(&p.p_, &pc);
    };

    // Calls fun on states that can be directly reached from this
    // state. Returns true immediately if fun returns true. Otherwise
    // returns false.
    bool do_valid_moves(const Map& map,
                        std::function<bool(State)> fun) const {
        static Direction dirs[] = {
            UP, RIGHT, DOWN, LEFT,
        };
        ObjMap<State> obj_map(*this, map);
        // Which objects overlap a teleporter before the move? (Needed
        // since teleporters behave as edge triggered).
        ObjMask tele_mask = teleporter_overlap(map, obj_map);
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            if (!snakes_[si].len_) {
                // This snake has already exited the level, can't move.
                continue;
            }
            // Make an ObjMap that's just like obj_map, but doesn't
            // have the current snake's tail filled in. (Needed since
            // the rules for how the tail is handled for movement vs
            // pushing are different).
            State push_st(*this);
            push_st.snakes_[si].len_--;
            ObjMap<State> push_map(push_st, map);

            for (auto dir : dirs) {
                // See what would happen when you move the current
                // snake in each of the directions. (Grow snake, move
                // snake to an empty space , or push one or more
                // objects).
                int delta = Setup::apply_direction(dir);
                int to = snakes_[si].i_[0] + delta;
                ObjMask pushed_ids = 0;
                int fruit_index = 0;
                if (is_valid_grow(map, obj_map, to, &fruit_index)) {
                    State new_state(*this);
                    new_state.snakes_[si].grow(dir);
                    new_state.delete_fruit(fruit_index);
                    if (new_state.process_gravity(map, tele_mask)) {
                        new_state.canonicalize(map);
                        if (fun(new_state)) {
                            return true;
                        }
                    }
                } if (is_valid_move(map, obj_map, to)) {
                    State new_state(*this);
                    new_state.snakes_[si].move(dir);
                    if (new_state.process_gravity(map, tele_mask)) {
                        new_state.canonicalize(map);
                        if (fun(new_state)) {
                            return true;
                        }
                    }
                } else if (is_valid_push(map, push_map,
                                         snake_id(si),
                                         snakes_[si].i_[0],
                                         delta,
                                         &pushed_ids)) {
                    State new_state(*this);
                    new_state.snakes_[si].move(dir);
                    new_state.do_pushes(obj_map, pushed_ids, delta);
                    if (new_state.process_gravity(map, tele_mask)) {
                        new_state.canonicalize(map);
                        if (fun(new_state)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    // Returns true if the state has reached the win condition.
    // (I.e. all Snakes have exited the level).
    bool win() {
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            if (snakes_[si].len_) {
                return false;
            }
        }

        return true;
    }

    // Prints the game state to stdout.
    void print(const Map& map) const {
        ObjMap<State, true> obj_map(*this, map);

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

private:
    friend Packed;
    friend ObjMap<State>;
    friend ObjMap<State, true>;

    static const uint16_t kGadgetDeleted = 0;

    static int empty_id() { return 0; }
    static int snake_id(int si) { return (1 + si); }
    static int gadget_id(int i) { return (1 + i + Setup::SnakeCount); }

    static ObjMask snake_mask(int si) { return 1 << si; }
    static ObjMask gadget_mask(int i) { return 1 << (Setup::SnakeCount + i); }

    // Returns true if the i'th fruit is still unconsumed.
    bool fruit_active(int i) const {
        return (fruit_ & (1 << i)) != 0;
    }

    // Mark the i'th fruit as consumed.
    void delete_fruit(int i) {
        fruit_ = fruit_ & ~(1 << i);
    }

    // Returns a mask of all objects that currently overlap a
    // teleporter. Each object will have one bit set for each
    // half of each teleporter-pair.
    ObjMask teleporter_overlap(const Map& map,
                               const ObjMap<State>& objmap) const {
        ObjMask mask = 0;
        const uint32_t width = Setup::ObjCount;
        for (int ti = 0; ti < Setup::TeleporterCount; ++ti) {
            mask |=
                ((objmap.mask_at(map.teleporters_[ti].first)) |
                 (objmap.mask_at(map.teleporters_[ti].second) << width))
                << (width * 2 * ti);
        }
        return mask;
    }

    // Returns true if a snake could move to the given location
    // without pushing anything.
    bool is_valid_move(const Map& map,
                       const ObjMap<State>& obj_map,
                       int to) const {
        if (obj_map.no_object_at(to) && empty_terrain_at(map, to)) {
            return true;
        }

        return false;
    }

    bool empty_terrain_at(const Map& map, int i) const {
        return map[i] == ' ';
    }

    // Returns true if a snake could grow by moving to the given
    // location. If that's the case, sets fruit_index to the
    // index of the fruit that would be eaten.
    bool is_valid_grow(const Map& map,
                       const ObjMap<State>& obj_map,
                       int to,
                       int* fruit_index) const {
        if (!obj_map.fruit_at(to)) {
            return false;
        }

        for (int fi = 0; fi < Setup::FruitCount; ++fi) {
            int fruit = map.fruit_[fi];
            if (fruit_active(fi) && fruit == to) {
                *fruit_index = fi;
                return true;
            }
        }

        assert(false);
    }

    // Returns true if the snake at the index pusher_id could push
    // an object at location push_at (with delta being the push
    // direction). Otherwise returns false.
    //
    // The bit corresponding to each pushed object will be toggled on
    // in pushed_ids.
    bool is_valid_push(const Map& map,
                       const ObjMap<State>& obj_map,
                       int pusher_id,
                       int push_at,
                       int delta,
                       ObjMask* pushed_ids) const __attribute__((noinline)) {
        int to = push_at + delta;

        if (// Nothing to push
            obj_map.no_object_at(to) ||
            // Object can't push itself.
            obj_map.id_at(to) == pusher_id ||
            // Can't push fruit.
            obj_map.fruit_at(to)) {
            return false;
        }

        // Initialize pushed_ids with the first candidate object
        // to push.
        *pushed_ids = obj_map.mask_at(to);
        bool again = true;

        // If object X is being pushed in direction D, and doing
        // that would require pushing object Y too, add Y to
        // the set as well. Repeat until no more objects get added
        // to the set during an iteration.
        //
        // If it turns out that one of the objects in the set is
        // not pushable, return false since the set as a whole can't
        // move.
        while (again) {
            again = false;
            for (int si = 0; si < Setup::SnakeCount; ++si) {
                if (*pushed_ids & snake_mask(si)) {
                    ObjMask new_pushed_ids = 0;
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
                    ObjMask new_pushed_ids = 0;
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

        // Don't allow the initiating object to push itself.
        return !(*pushed_ids & snake_mask(pusher_id - 1));
    }

    // Returns false if the snake at index si definitely can't be
    // pushed in the direction of delta. If pushing the object would
    // require pushing another object, mark that object in the
    // pushed_ids mask.
    bool snake_can_be_pushed(const Map& map,
                             const ObjMap<State>& obj_map,
                             int si,
                             int delta,
                             ObjMask* pushed_ids) const __attribute__((noinline)) {
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

    // Returns false if the gadget at index gi definitely can't be
    // pushed in the direction of delta. If pushing the object would
    // require pushing another object, mark that object in the
    // pushed_ids mask.
    bool gadget_can_be_pushed(const Map& map,
                              const ObjMap<State>& obj_map,
                              int gi,
                              int delta,
                              ObjMask* pushed_ids) const __attribute__((noinline)) {
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

    // Move all objects that are toggled in pushed_ids in the
    // direction push_delta.
    void do_pushes(const ObjMap<State>& obj_map,
                   ObjMask pushed_ids, int push_delta) {
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

    // Process the "physics", iteratively until there are no more
    // changes.
    //
    // - Snakes that overlap an exit leave the map.
    // - Objects that overlap a teleporter they didn't overlap
    //   previously will teleport.
    // - Objects that are not supported by the ground or a fruit
    //   will drop down a state.
    bool process_gravity(const Map& map, ObjMask orig_tele_mask)
        __attribute__((noinline)) {
        // A mask with a bit set for each object that might still
        // be falling. Once an object reaches terra firma, it's
        // bit is set to zero and it can be ignored on further
        // iterations.
        ObjMask recompute_falling = mask_n_bits(Setup::ObjCount);
        // For each object O, contains a mask of the objects O' that
        // are supporting this object. (I.e. O can only be falling
        // if O' is also falling).
        ObjMask falling[Setup::ObjCount];

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
        ObjMap<State> obj_map(*this, map);
        ObjMask new_tele_mask = teleporter_overlap(map, obj_map);
        if (new_tele_mask & ~orig_tele_mask) {
            if (process_teleports(map, obj_map, orig_tele_mask,
                                  new_tele_mask)) {
                orig_tele_mask = teleporter_overlap(map,
                                                    ObjMap<State>(*this, map));
                goto again;
            }
        }
        orig_tele_mask = new_tele_mask;

        // Recompute the supports for each object.
        //
        // supported will be set to 1 for any objects that are now on
        // the ground. For objects that aren't, falling will be
        // updated with a new mask of possibly supporting objects.
        ObjMask supported = 0;
        for (int si = 0; si < Setup::SnakeCount; ++si) {
            ObjMask mask = 0;
            if (snakes_[si].len_) {
                if (recompute_falling & snake_mask(si)) {
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
            ObjMask mask = 0;
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

        // Expand the supported set: for any object O that is not
        // supported yet, check if any of its supporting objects O'
        // are in the supported set. If at least one is, add O to
        // the supported set.
        //
        // Iterate until no more objects are being added to the set.
        while (1) {
            bool again = false;
            for (int i = 0; i < Setup::ObjCount; ++i) {
                ObjMask mask = 1 << i;
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

        // Anything not in the supported set gets pushed down one
        // step.
        ObjMask to_push = mask_n_bits(Setup::ObjCount)
            & ~supported;

        if (to_push) {
            do_pushes(obj_map, to_push, Setup::W);
            // If the object dropping into the hazard would cause
            // a game over situation, bail out.
            if (destroy_if_intersects_hazard(map, obj_map, to_push)) {
                return false;
            }
            // Recompute the situation for the objects that fell down
            // this turn.
            recompute_falling |= to_push;
            goto again;
        }

        return true;
    }

    // For each object that's just moved to a teleporter (i.e. is
    // in new_tele_mask but not in orig_tele_mask), move the
    // object to the other side if possible.
    //
    // Returns true if any objects were teleported.
    bool process_teleports(const Map& map, const ObjMap<State>& obj_map,
                           ObjMask orig_tele_mask,
                           ObjMask new_tele_mask) {
        ObjMask only_new = new_tele_mask & ~orig_tele_mask;
        ObjMask test = 1;
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

    // Check if the snake would fit on the map if it were moved by the
    // given delta. If so, move it and return true. Otherwise return
    // false.
    bool try_snake_teleport(const Map& map,
                            const ObjMap<State>& obj_map,
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

    // Check if the gadget would fit on the map if it were moved by
    // the given delta. If so, move it and return true. Otherwise
    // return false.
    bool try_gadget_teleport(const Map& map,
                             const ObjMap<State>& obj_map,
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

    // Check whether any of the objects whose bits are set in the
    // pushed_ids mask is intersecting a hazard.
    //
    // - A snake intersecting a hazard is a game over.
    // - A gadget intersecting a hazard just gets destroyed.
    //
    // Returns true iff game over.
    bool destroy_if_intersects_hazard(const Map& map,
                                      const ObjMap<State>& obj_map,
                                      ObjMask pushed_ids) {
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

    // It is possible for two states to be functionally equal but
    // have a different internal representation. This happens
    // since two snakes of the same size, or two gadgets of the
    // same shape are interchangable. Their exact identity never
    // matters for the solution.
    //
    // Transforms the state (in-place) such that if S and S' are
    // functionally equal but S != S', canonicalize(S) ==
    // canonicalize(S').
    void canonicalize(const Map& map) {
        // Sort the snakefs.
        std::sort(&snakes_[0], &snakes_[Setup::SnakeCount]);
        if (Setup::GadgetCount > 0) {
            // Sort the gadgets primarily by shape, secondarily
            // by offset.
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

    // Checks whether any snakes are in a position to exit the map.
    // If so, marks them as having exited by setting the snake's
    // length to 0.
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

    // If the snake is supported by the ground, returns zero.
    // Otherwise returns a bitmask with 1 set for each object
    // that might be supporting the snake + the snake itself.
    ObjMask is_snake_falling(const Map& map,
                             const ObjMap<State>& obj_map,
                             int si) const {
        const Snake& snake = snakes_[si];
        ObjMask pushed_ids = snake_mask(si);

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

    // If the gadget is supported by the ground or a spike, returns
    // zero. Otherwise returns a bitmask with 1 set for each object
    // that might be supporting the gadget + the gadget itself.
    ObjMask is_gadget_falling(const Map& map,
                              const ObjMap<State>& obj_map,
                              int gi) const {
        const auto& gadget = map.gadgets_[gi];

        ObjMask pushed_ids = gadget_mask(gi);
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

    // Returns true if a segment of the snake is located in a spike or
    // a water location.
    bool snake_intersects_hazard(const Map& map, const Snake& snake) const {
        for (int i = 0; i < snake.len_; ++i) {
            int at = snake.i_[i];
            if (map[at] == '~' || map[at] == '#')
                return true;
        }

        return false;
    }

    // Returns true if a part of the gadget is located in a water
    // location.
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

    // De-serialize the state.
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

    // Serialize the state.
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

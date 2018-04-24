#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <sparsehash/dense_hash_map>
#include <sparsehash/sparse_hash_map>
#include <third-party/cityhash/city.h>
#include <unordered_map>
#include <vector>

enum Direction {
    UP, RIGHT, DOWN, LEFT,
};

template<int H, int W, int MaxLen>
class Snake {
public:
    static const int kDirWidth = 2;
    static const int kDirMask = (1 << kDirWidth) - 1;

    Snake() : tail_(0), i_(0), len_(0), id_(0) {
    }

    Snake(int id, int i)
        : tail_(0),
          i_(i),
          len_(1),
          id_(id) {
        assert(i_ < H * W);
    }

    void grow(Direction dir) {
        i_ += apply_direction(dir);
        ++len_;
        tail_ = (tail_ << kDirWidth) | dir;
    }

    void move(Direction dir) {
        i_ += apply_direction(dir);
        tail_ &= ~(kDirMask << ((len_ - 2) * kDirWidth));
        tail_ = (tail_ << kDirWidth) | dir;
    }

    Direction tail(int i) const {
        return Direction((tail_ >> (i * kDirWidth)) & kDirMask);
    }

    static int apply_direction(Direction dir) {
        static int deltas[] = { -W, 1, W, -1 };
        return deltas[dir];
    }

    bool operator==(const Snake<H, W, MaxLen> other) const {
        return tail_ == other.tail_ &&
            i_ == other.i_ &&
            len_ == other.len_;
    }

    uint32_t tail_;
    uint16_t i_;
    uint8_t len_;
    uint8_t id_;
};

template<int H, int W>
class Gadget {
public:
    Gadget() : size_(0) {
    }

    void add(int i) {
        i_[size_++] = i;
    }

    uint16_t size_;
    uint16_t i_[8];
};

template<int H, int W, int FruitCount, int SnakeCount, int SnakeMaxLen,
         int GadgetCount=0>
class State {
public:
    using Snake = typename ::Snake<H, W, SnakeMaxLen>;
    using Gadget = typename ::Gadget<H, W>;

    static const int16_t kGadgetDeleted = 1 << 15;

    class Map {
    public:
        explicit Map(const char* base_map) : exit_(0) {
            assert(strlen(base_map) == H * W);
            base_map_ = new char[H * W];
            int fruit_count = 0;
            int snake_count = 0;
            int max_len = 0;
            for (int i = 0; i < H * W; ++i) {
                if (base_map[i] == 'O') {
                    if (FruitCount) {
                        fruit_[fruit_count++] = i;
                    }
                    base_map_[i] = ' ';
                } else if (base_map[i] == '*') {
                    assert(!exit_);
                    base_map_[i] = ' ';
                    exit_ = i;
                } else if (base_map[i] == 'R' ||
                           base_map[i] == 'G' ||
                           base_map[i] == 'B') {
                    base_map_[i] = ' ';
                    Snake snake = Snake(base_map[i], i);
                    int len = 0;
                    snake.tail_ = trace_tail(base_map, i, &len);
                    snake.len_ += len;
                    snakes_[snake_count++] = snake;
                    max_len = std::max(max_len, (int) snake.len_);
                } else if (isdigit(base_map[i])) {
                    base_map_[i] = ' ';
                    gadgets_[base_map[i] - '0'].add(i);
                } else if (base_map[i] == '>' ||
                           base_map[i] == '<' ||
                           base_map[i] == '^' ||
                           base_map[i] == 'v') {
                    base_map_[i] = ' ';
                } else {
                    base_map_[i] = base_map[i];
                }
            }

            if (SnakeMaxLen < max_len + FruitCount) {
                fprintf(stderr, "Expected SnakeMaxLen >= %d, got %d\n",
                        max_len + FruitCount,
                        SnakeMaxLen);
            }
            assert(fruit_count == FruitCount);
            assert(snake_count == SnakeCount);
            assert(exit_);
        }

        uint32_t trace_tail(const char* base_map, int i, int* len) {
            if (base_map[i - 1] == '>') {
                ++*len;
                return RIGHT |
                    (trace_tail(base_map, i - 1, len) << Snake::kDirWidth);
            }
            if (base_map[i + 1] == '<') {
                ++*len;
                return LEFT |
                    (trace_tail(base_map, i + 1, len) << Snake::kDirWidth);
            }
            if (base_map[i - W] == 'v') {
                ++*len;
                return DOWN |
                    (trace_tail(base_map, i - W, len) << Snake::kDirWidth);
            }
            if (base_map[i + W] == '^') {
                ++*len;
                return UP |
                    (trace_tail(base_map, i + W, len) << Snake::kDirWidth);
            }

            return 0;
        }

        char operator[](int i) const {
            return this->base_map_[i];
        }

        char* base_map_;
        int exit_;
        int fruit_[FruitCount];
        Snake snakes_[SnakeCount];
        Gadget gadgets_[GadgetCount];
    };


    State() :
        win_(0),
        fruit_((1 << FruitCount) - 1) {
        for (auto& offset : gadget_offset_) {
            offset = 0;
        }
    }

    State(const Map& map) : State() {
        for (int i = 0; i < SnakeCount; ++i) {
            snakes_[i] = map.snakes_[i];
        }
    }

    void print(const Map& map) {
        char obj_map[H * W];
        draw_objs(map, obj_map, true);

#if 0
        for (auto snake : snakes_) {
            printf("len=%d, i=%d, tail=", snake.len_, snake.i_);
            for (int j = 0; j < snake.len_ - 1; ++j) {
                printf(" %d", snake.tail(j));
            }
            printf("\n");
        }
#endif

        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                int l = i * W + j;
                if (obj_map[l]) {
                    printf("%c", obj_map[l]);
                } else if (l == map.exit_) {
                    printf("*");
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

    bool fruit_active(int i) {
        return (fruit_ & (1 << i)) != 0;
    }

    void do_valid_moves(const Map& map,
                        std::function<bool(State,
                                           const Snake&,
                                           Direction)> fun) {
        static Direction dirs[] = {
            UP, RIGHT, DOWN, LEFT,
        };
        char obj_map[H * W];
        draw_objs(map, obj_map, false);
        for (int si = 0; si < SnakeCount; ++si) {
            if (!snakes_[si].len_) {
                continue;
            }
            // There has to be a cleaner way to do this...
            snakes_[si].len_--;
            char push_map[H * W];
            draw_objs(map, push_map, false);
            snakes_[si].len_++;
            for (auto dir : dirs) {
                int delta = Snake::apply_direction(dir);
                int to = snakes_[si].i_ + delta;
                int pushed_ids = 0;
                int fruit_index = 0;
                if (is_valid_grow(map, to, &fruit_index)) {
                    State new_state(*this);
                    new_state.snakes_[si].grow(dir);
                    new_state.delete_fruit(fruit_index);
                    if (new_state.process_gravity(map)) {
                        if (fun(new_state, snakes_[si], dir)) {
                            return;
                        }
                    }
                } else if (is_valid_move(map, obj_map, to)) {
                    State new_state(*this);
                    new_state.snakes_[si].move(dir);
                    if (new_state.process_gravity(map)) {
                        if (fun(new_state, snakes_[si], dir)) {
                            return;
                        }
                    }
                } else if (is_valid_push(map, push_map,
                                         snake_id(si),
                                         snakes_[si].i_,
                                         delta,
                                         &pushed_ids) &&
                           !(pushed_ids & (1 << si))) {
                    State new_state(*this);
                    new_state.snakes_[si].move(dir);
                    new_state.do_pushes(pushed_ids, delta);
                    // printf("++\n");
                    // print(map);
                    // new_state.print(map);
                    if (new_state.process_gravity(map)) {
                        if (fun(new_state, snakes_[si], dir)) {
                            return;
                        }
                    }
                }
            }
        }
    }

    void do_pushes(int pushed_ids, int push_delta) {
        for (int i = 0; i < SnakeCount; ++i) {
            if (pushed_ids & (1 << i)) {
                snakes_[i].i_ += push_delta;
            }
        }
        for (int i = 0; i < GadgetCount; ++i) {
            if (pushed_ids & (1 << (i + SnakeCount))) {
                gadget_offset_[i] += push_delta;
            }
        }
    }

    bool destroy_if_intersects_hazard(const Map& map, int pushed_ids) {
        for (int i = 0; i < SnakeCount; ++i) {
            if (pushed_ids & (1 << i)) {
                if (snake_intersects_hazard(map, snakes_[i]))
                    return true;
            }
        }
        for (int i = 0; i < GadgetCount; ++i) {
            if (pushed_ids & (1 << (i + SnakeCount))) {
                if (gadget_intersects_hazard(map, i)) {
                    gadget_offset_[i] = kGadgetDeleted;
                }
            }
        }
        return false;
    }

    bool is_valid_grow(const Map& map,
                       int to,
                       int* fruit_index) {
        for (int i = 0; i < FruitCount; ++i) {
            int fruit = map.fruit_[i];
            if (fruit_active(i) &&
                fruit == to) {
                *fruit_index = i;
                return true;
            }
        }

        return false;
    }

    bool is_valid_move(const Map& map,
                       const char* obj_map,
                       int to) {
        if (no_object_at(obj_map, to) && empty_terrain_at(map, to)) {
            return true;
        }

        return false;
    }

    bool empty_terrain_at(const Map& map, int i) {
        return map[i] == ' ';
    }

    bool no_object_at(const char* obj_map, int i) {
        return !obj_map[i];
    }

    bool fruit_at(const char* obj_map, int i) {
        return obj_map[i] == 'Q';
    }

    bool is_valid_push(const Map& map,
                       const char* obj_map,
                       int pusher_id,
                       int push_at,
                       int delta,
                       int* pushed_ids) {
        int to = push_at + delta;

        if (no_object_at(obj_map, to) ||
            obj_map[to] == pusher_id ||
            fruit_at(obj_map, to)) {
            return false;
        }

        int index = map_id_to_index(obj_map[to]);
        *pushed_ids = 1 << index;

        bool again = true;

        while (again) {
            again = false;
            for (int si = 0; si < SnakeCount; ++si) {
                if (*pushed_ids & (1 << si)) {
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
            for (int i = 0; i < GadgetCount; ++i) {
                if (*pushed_ids & (1 << (SnakeCount + i))) {
                    int new_pushed_ids = 0;
                    if (!gadget_can_be_pushed(map,
                                              obj_map,
                                              i,
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

    bool foreign_object_at(const char* obj_map, int i, int id) {
        return !no_object_at(obj_map, i) &&
            obj_map[i] != id;
    }

    bool snake_can_be_pushed(const Map& map,
                             const char* obj_map,
                             int si,
                             int delta,
                             int* pushed_ids) {
        const Snake& snake = snakes_[si];
        // The space the Snake's head would be pushed to.
        int to = snake.i_ + delta;

        for (int j = 0; j < snake.len_; ++j) {
            if (!empty_terrain_at(map, to)) {
                return false;
            }
            if (fruit_at(obj_map, to)) {
                return false;
            }
            if (foreign_object_at(obj_map, to, snake_id(si))) {
                *pushed_ids |= 1 << map_id_to_index(obj_map[to]);
            }
            to -= Snake::apply_direction(snake.tail(j));
        }

        return true;
    }

    bool gadget_can_be_pushed(const Map& map,
                              const char* obj_map,
                              int gadget_index,
                              int delta,
                              int* pushed_ids) {
        const auto& gadget = map.gadgets_[gadget_index];
        int offset = gadget_offset_[gadget_index];

        for (int j = 0; j < gadget.size_; ++j) {
            int i = gadget.i_[j] + offset + delta;
            if (!empty_terrain_at(map, i)) {
                return false;
            }
            if (fruit_at(obj_map, i)) {
                return false;
            }
            if (!no_object_at(obj_map, i)) {
                *pushed_ids |= 1 << map_id_to_index(obj_map[i]);
            }
        }

        return true;
    }

    int map_id_to_index(int id) {
        for (int si = 0; si < SnakeCount; ++si) {
            if (id == snake_id(si)) {
                return si;
            }
        }
        for (int i = 0; i < GadgetCount; ++i) {
            if (id == '0' + i) {
                return SnakeCount + i;
            }
        }
        printf("%c\n", id);
        assert(false);
        return 0;
    }

    bool process_gravity(const Map& map) {
        char obj_map[H * W];

    again:
        check_exits(map);
        draw_objs(map, obj_map, false);
        for (int si = 0; si < SnakeCount; ++si) {
            if (snakes_[si].len_) {
                int falling = is_snake_falling(map, obj_map, si);
                if (falling) {
                    do_pushes(falling, W);
                    if (destroy_if_intersects_hazard(map, falling))
                        return false;
                    goto again;
                }
            }
        }

        for (int i = 0; i < GadgetCount; ++i) {
            int offset = gadget_offset_[i];
            if (offset != kGadgetDeleted) {
                int falling = is_gadget_falling(map, obj_map, i);
                if (falling) {
                    do_pushes(falling, W);
                    if (destroy_if_intersects_hazard(map, falling))
                        return false;
                    goto again;
                }
            }
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
                    snake.i_ = 0;
                    snake.tail_ = 0;
                    update_win();
                }
            }
        }
    }

    void update_win() {
        for (int si = 0; si < SnakeCount; ++si) {
            if (snakes_[si].len_) {
                win_ = 0;
                return;
            }
        }

        win_ = 1;
    }

    int is_snake_falling(const Map& map,
                         const char* obj_map,
                         int si) {
        const Snake& snake = snakes_[si];
        // The space below the snake's head.
        int below = snake.i_ + W;
        int pushed_ids = 1 << map_id_to_index(snake_id(si));

        for (int j = 0; j < snake.len_; ++j) {
            if (map[below] == '.') {
                return 0;
            }
            if (foreign_object_at(obj_map, below, snake_id(si))) {
                int new_pushed_ids = 0;
                if (is_valid_push(map, obj_map,
                                  snake_id(si),
                                  below - W, W,
                                  &new_pushed_ids)) {
                    pushed_ids |= new_pushed_ids;
                } else {
                    return 0;
                }
            }

            below -= Snake::apply_direction(snake.tail(j));
        }

        return pushed_ids;
    }

    int is_gadget_falling(const Map& map,
                          const char* obj_map,
                          int gadget_index) {
        const auto& gadget = map.gadgets_[gadget_index];

        int pushed_ids = 1 << (gadget_index + SnakeCount);
        int id = ('0' + gadget_index);

        for (int j = 0; j < gadget.size_; ++j) {
            int at = gadget.i_[j] + gadget_offset_[gadget_index];
            int below = at + W;
            if (map[below] == '.') {
                return 0;
            }
            if (map[below] == '#') {
                return 0;
            }
            if (foreign_object_at(obj_map, below, id)) {
                int new_pushed_ids = 0;
                if (is_valid_push(map, obj_map, id, at, W, &new_pushed_ids)) {
                    pushed_ids |= new_pushed_ids;
                } else {
                    return 0;
                }
            }
        }

        return pushed_ids;
    }

    bool snake_head_at_exit(const Map& map, const Snake& snake) {
        // Only the head of the snake will trigger an exit
        return snake.i_ == map.exit_;
    }

    bool snake_intersects_hazard(const Map& map, const Snake& snake) {
        int i = snake.i_;
        for (int j = 0; j < snake.len_; ++j) {
            if (map[i] == '~' || map[i] == '#')
                return true;
            i -= Snake::apply_direction(snake.tail(j));
        }

        return false;
    }

    bool gadget_intersects_hazard(const Map& map,
                                  int gadget_index) {
        int offset = gadget_offset_[gadget_index];
        if (offset == kGadgetDeleted)
            return false;
        const auto& gadget = map.gadgets_[gadget_index];
        for (int j = 0; j < gadget.size_; ++j) {
            // Spikes aren't a hazard for gadgets.
            if (map[gadget.i_[j] + offset] == '~')
                return true;
        }

        return false;
    }

    void draw_objs(const Map& map, char* obj_map,
                   bool draw_path) {
        memset(obj_map, 0, H * W);
        for (int si = 0; si < SnakeCount; ++si) {
            draw_snake(obj_map, si, draw_path);
        }
        for (int i = 0; i < FruitCount; ++i) {
            if (fruit_active(i)) {
                obj_map[map.fruit_[i]] = 'Q';
            }
        }
        for (int i = 0; i < GadgetCount; ++i) {
            int offset = gadget_offset_[i];
            if (offset != kGadgetDeleted) {
                const auto& gadget = map.gadgets_[i];
                for (int j = 0; j < gadget.size_; ++j) {
                    obj_map[offset + gadget.i_[j]] = '0' + i;
                }
            }
        }
    }

    int snake_id(int si) {
        return snakes_[si].id_;
    }

    void draw_snake(char* obj_map, int si, bool draw_path) {
        const Snake& snake = snakes_[si];
        int i = snake.i_;
        for (int j = 0; j < snake.len_; ++j) {
            if (j == 0 || !draw_path) {
                obj_map[i] = snake_id(si);
            } else {
                switch (snake.tail(j - 1)) {
                case UP: obj_map[i] = '^'; break;
                case DOWN: obj_map[i] = 'v'; break;
                case LEFT: obj_map[i] = '<'; break;
                case RIGHT: obj_map[i] = '>'; break;
                }
            }
            i -= Snake::apply_direction(snake.tail(j));
        }
    }

    bool operator==(const State<H, W, FruitCount, SnakeCount, SnakeMaxLen, GadgetCount> other) const {
        for (int i = 0; i < SnakeCount; ++i) {
            if (!(snakes_[i] == other.snakes_[i])) {
                return false;
            }
        }
        for (int i = 0; i < GadgetCount; ++i) {
            if (gadget_offset_[i] != other.gadget_offset_[i]) {
                return false;
            }
        }
        if (win_ != other.win_ ||
            fruit_ != other.fruit_) {
            return false;
        }
        return true;
    }

    Snake snakes_[SnakeCount];
    int16_t gadget_offset_[GadgetCount + GadgetCount % 2];
    uint16_t win_;
    uint16_t fruit_;
};

template<class T>
struct hash {
    uint64_t operator()(const T& key) const {
        return CityHash64(reinterpret_cast<const char*>(&key),
                          sizeof(key));
        // return CityHash64(reinterpret_cast<const char*>(&key.snakes_),
        //                   sizeof(key.snakes_));
        // CityHash64(reinterpret_cast<const char*>(&key.fruit_),
        //            sizeof(key.fruit_));
        // const uint32_t* data = reinterpret_cast<const uint32_t*>(&key);
        // return xxhash32<sizeof(key) / 4>::scalar(data, 0x8fc5249f);
    }
};

template<class T>
struct eq {
    bool operator()(const T& a, const T& b) const {
        return a == b;
    }
};

template<class St, class Map>
int search(St start_state, const Map& map) {
    printf("%ld\n", sizeof(St));
    St null_state;

    // Just in case the starting state is invalid.
    start_state.process_gravity(map);

    // BFS state
    std::deque<St> todo;
    // std::unordered_map<St, St, hash<St>, eq<St>> seen_states;
    // google::dense_hash_map<St, St, hash<St>, eq<St>> seen_states;
    // seen_states.set_empty_key(null_state);
    google::sparse_hash_map<St, St, hash<St>, eq<St>> seen_states;
    size_t steps = 0;
    St win_state = null_state;
    bool win = false;

    todo.push_back(start_state);
    seen_states[start_state] = null_state;

    while (!todo.empty() && !win) {
        auto st = todo.front();
        todo.pop_front();

        ++steps;
        if (!(steps & 0xffff)) {
            printf(".");
            fflush(stdout);
        }

        // printf("%lx [%lx]\n", hash<St>()(st),
        //        hash<St>()(seen_states[st]));
        // st.print(map);

        st.do_valid_moves(map,
                          [&st, &todo, &seen_states, &win, &map,
                           &win_state](St new_state,
                                       const typename St::Snake& snake,
                                       Direction dir) {
                if (seen_states.find(new_state) != seen_states.end()) {
                    return false;
                }

                seen_states[new_state] = st;
                todo.push_back(new_state);

                if (new_state.win_) {
                    win_state = new_state;
                    win = true;
                    return true;
                }
                return false;
            });
    }

    printf("%s\n",
           win ? "Win" : "No solution");

    int moves = 0;
    if (win) {
        while (!(win_state == start_state)) {
            win_state.print(map);
            win_state = seen_states[win_state];
            ++moves;
        }
    }

    printf("%ld states, %d moves\n", steps, moves);

    return moves;
}

#include "level00.h"
#include "level01.h"
#include "level02.h"
#include "level03.h"
#include "level04.h"
#include "level05.h"
#include "level06.h"
#include "level07.h"
#include "level08.h"
#include "level09.h"
#include "level10.h"
#include "level11.h"
#include "level12.h"
#include "level13.h"
#include "level14.h"
#include "level15.h"
#include "level16.h"
#include "level17.h"
#include "level18.h"
#include "level19.h"
#include "level20.h"
#include "level21.h"
#include "level22.h"
#include "level23.h"
#include "level24.h"
#include "level25.h"
#include "level26.h"
#include "level27.h"
#include "level28.h"
#include "level29.h"
#include "level30.h"
#include "level31.h"
#include "level32.h"
#include "level33.h"
#include "level34.h"
#include "level35.h"
#include "level36.h"
#include "level37.h"
#include "level38.h"
#include "level39.h"
#include "level40.h"
#include "level41.h"
#include "level42.h"
#include "level43.h"
#include "level44.h"
#include "level45.h"
#include "levelstar1.h"
// #include "levelstar2.h"
#include "levelstar3.h"
#include "levelstar4.h"
#include "levelstar5.h"
#include "levelstar6.h"

int main() {
#define EXPECT_EQ(wanted, actual)                                       \
    do {                                                                \
        auto tmp = actual;                                              \
        if (tmp != wanted) {                                            \
            fprintf(stderr, "Error: expected %s => %d, got %d\n", #actual, wanted, tmp); \
        } \
    } while (0);

    EXPECT_EQ(29, level_00());
    EXPECT_EQ(16, level_01());
    EXPECT_EQ(25, level_02());
    EXPECT_EQ(27, level_03());
    EXPECT_EQ(30, level_04());
    EXPECT_EQ(24, level_05());
    EXPECT_EQ(36, level_06());
    EXPECT_EQ(43, level_07());
    EXPECT_EQ(29, level_08());
    EXPECT_EQ(37, level_09());
    EXPECT_EQ(33, level_10());
    EXPECT_EQ(35, level_11());
    EXPECT_EQ(52, level_12());
    EXPECT_EQ(44, level_13());
    EXPECT_EQ(24, level_14());
    EXPECT_EQ(34, level_15());
    EXPECT_EQ(65, level_16());
    EXPECT_EQ(68, level_17());
    EXPECT_EQ(35, level_18());
    EXPECT_EQ(47, level_19());
    EXPECT_EQ(50, level_20());
    EXPECT_EQ(39, level_21());
    EXPECT_EQ(45, level_22());
    EXPECT_EQ(53, level_23());
    EXPECT_EQ(26, level_24());
    EXPECT_EQ(35, level_25());
    EXPECT_EQ(35, level_26());
    EXPECT_EQ(49, level_27());
    EXPECT_EQ(49, level_28());
    // OOM
    // EXPECT_EQ(0, level_29());
    // Teleport
    // EXPECT_EQ(15, level_30());
    // Teleport
    // EXPECT_EQ(8, level_31());
    // Teleport
    // EXPECT_EQ(21, level_32());
    // Teleport
    // EXPECT_EQ(42, level_33());
    // Teleport
    // EXPECT_EQ(17, level_34());
    // Teleport
    // EXPECT_EQ(29, level_35());
    // Teleport
    // EXPECT_EQ(29, level_36());
    // Teleport
    // EXPECT_EQ(16, level_37());
    // Teleport
    // EXPECT_EQ(28, level_38());
    EXPECT_EQ(53, level_39());
    EXPECT_EQ(51, level_40());
    EXPECT_EQ(34, level_41());
    EXPECT_EQ(42, level_42());
    EXPECT_EQ(36, level_43());
    // Teleport
    // EXPECT_EQ(0, level_44());
    EXPECT_EQ(77, level_45());
    // OOM
    // EXPECT_EQ(0, level_star1());
    // Too many fruit... Need to parametrize type of fruit_
    // EXPECT_EQ(60, level_star2());
    // OOM
    // EXPECT_EQ(0, level_star3());
    EXPECT_EQ(44, level_star4());
    // Teleport
    // EXPECT_EQ(69, level_star5());
    // OOM
    // EXPECT_EQ(0, level_star6());

    return 0;
}

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <sparsehash/dense_hash_map>
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
        for (int i = 0; i < GadgetCount; ++i) {
            gadget_offset_[i] = 0;
        }
    }

    State(const Map& map) : State() {
        for (int i = 0; i < SnakeCount; ++i) {
            snakes_[i] = map.snakes_[i];
        }
    }

    void print(const Map& map) {
        char snake_map[H * W];
        draw_snakes(map, snake_map, true);

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
                if (snake_map[l]) {
                    printf("%c", snake_map[l]);
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

    int add_snake(const Snake& snake, int i) {
        assert(i < SnakeCount);
        snakes_[i] = snake;
        return i + 1;
    }

    // int add_fruit(int r, int c, int i) {
    //     assert(i < FruitCount);
    //     fruit_[i] = r * W + c;
    //     return i + 1;
    // }

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
        char snake_map[H * W];
        draw_snakes(map, snake_map, false);
        for (int s = 0; s < SnakeCount; ++s) {
            if (!snakes_[s].len_) {
                continue;
            }
            // There has to be a cleaner way to do this...
            snakes_[s].len_--;
            char push_map[H * W];
            draw_snakes(map, push_map, false);
            snakes_[s].len_++;
            for (auto dir : dirs) {
                int delta = Snake::apply_direction(dir);
                int to = snakes_[s].i_ + delta;
                int pushed_ids = 0;
                int fruit_index = 0;
                if (is_valid_grow(map, snakes_[s], to, &fruit_index)) {
                    State new_state(*this);
                    new_state.snakes_[s].grow(dir);
                    new_state.delete_fruit(fruit_index);
                    if (new_state.process_gravity(map)) {
                        if (fun(new_state, snakes_[s], dir)) {
                            return;
                        }
                    }
                } else if (is_valid_move(map, snake_map, snakes_[s], to)) {
                    State new_state(*this);
                    new_state.snakes_[s].move(dir);
                    if (new_state.process_gravity(map)) {
                        if (fun(new_state, snakes_[s], dir)) {
                            return;
                        }
                    }
                } else if (is_valid_push(map, push_map,
                                         snakes_[s].id_,
                                         snakes_[s].i_,
                                         delta,
                                         &pushed_ids) &&
                           !(pushed_ids & (1 << s))) {
                    State new_state(*this);
                    new_state.snakes_[s].move(dir);
                    new_state.do_pushes(pushed_ids, delta);
                    // printf("++\n");
                    // print(map);
                    // new_state.print(map);
                    if (new_state.process_gravity(map)) {
                        if (fun(new_state, snakes_[s], dir)) {
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

    bool destroy_if_intersects_water(const Map& map, int pushed_ids) {
        for (int i = 0; i < SnakeCount; ++i) {
            if (pushed_ids & (1 << i)) {
                if (snake_intersects_water(map, snakes_[i]))
                    return true;
            }
        }
        for (int i = 0; i < GadgetCount; ++i) {
            if (pushed_ids & (1 << (i + SnakeCount))) {
                if (gadget_intersects_water(map, i)) {
                    gadget_offset_[i] = kGadgetDeleted;
                }
            }
        }
        return false;
    }

    bool is_valid_grow(const Map& map,
                       const Snake& snake,
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
                       const char* snake_map,
                       const Snake& snake, int to) {
        if (!snake_map[to] && map[to] == ' ') {
            return true;
        }

        return false;
    }

    bool is_valid_push(const Map& map,
                       const char* snake_map,
                       int pusher_id,
                       int push_at,
                       int delta,
                       int* pushed_ids) {
        int to = push_at + delta;

        if (!snake_map[to] || snake_map[to] == pusher_id ||
            snake_map[to] == 'Q') {
            return false;
        }

        int index = map_id_to_index(snake_map[to]);
        *pushed_ids = 1 << index;

        bool again = true;

        while (again) {
            again = false;
            for (int i = 0; i < SnakeCount; ++i) {
                if (*pushed_ids & (1 << i)) {
                    int new_pushed_ids = 0;
                    if (!snake_can_be_pushed(map, snake_map,
                                             snakes_[i],
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
                                              snake_map,
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

    bool snake_can_be_pushed(const Map& map,
                             const char* snake_map,
                             const Snake& snake,
                             int delta,
                             int* pushed_ids) {
        // The space the Snake's head would be pushed to.
        int to = snake.i_ + delta;

        for (int j = 0; j < snake.len_; ++j) {
            if (map[to] != ' ') {
                return false;
            }
            if (snake_map[to] == 'Q') {
                return false;
            }
            if (snake_map[to] && snake_map[to] != snake.id_) {
                *pushed_ids |= 1 << map_id_to_index(snake_map[to]);
            }
            to -= Snake::apply_direction(snake.tail(j));
        }

        return true;
    }

    bool gadget_can_be_pushed(const Map& map,
                              const char* snake_map,
                              int gadget_index,
                              int delta,
                              int* pushed_ids) {
        const auto& gadget = map.gadgets_[gadget_index];
        int offset = gadget_offset_[gadget_index];

        for (int j = 0; j < gadget.size_; ++j) {
            int i = gadget.i_[j] + offset + delta;
            if (map[i] != ' ') {
                return false;
            }
            if (snake_map[i] == 'Q') {
                return false;
            }
            if (snake_map[i]) {
                *pushed_ids |= 1 << map_id_to_index(snake_map[i]);
            }
        }

        // print(map);

        return true;
    }

    int map_id_to_index(int id) {
        for (int i = 0; i < SnakeCount; ++i) {
            if (id == snakes_[i].id_) {
                return i;
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
        char snake_map[H * W];

    again:
        check_exits(map);
        draw_snakes(map, snake_map, false);
        for (auto& snake : snakes_) {
            if (snake.len_) {
                bool falling, falling_to_death;
                int pushed_while_falling = 0;
                is_snake_falling(map,
                                 snake_map,
                                 snake,
                                 &falling,
                                 &falling_to_death,
                                 &pushed_while_falling);
                if (falling) {
                    if (falling_to_death) {
                        return false;
                    } else {
                        do_pushes(pushed_while_falling, W);
                        if (destroy_if_intersects_water(map,
                                                        pushed_while_falling))
                            return false;
                        goto again;
                    }
                }
            }
        }

        for (int i = 0; i < GadgetCount; ++i) {
            int offset = gadget_offset_[i];
            if (offset != kGadgetDeleted) {
                bool falling, falling_to_death;
                int pushed_while_falling = 0;
                is_gadget_falling(map,
                                  snake_map,
                                  i,
                                  &falling,
                                  &falling_to_death,
                                  &pushed_while_falling);
                if (falling) {
                    if (falling_to_death) {
                        gadget_offset_[i] = kGadgetDeleted;
                    } else {
                        do_pushes(pushed_while_falling, W);
                        if (destroy_if_intersects_water(map,
                                                        pushed_while_falling))
                            return false;
                        goto again;
                    }
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
                if (snake_intersects_exit(map, snake)) {
                    snake.len_ = 0;
                    snake.i_ = 0;
                    snake.tail_ = 0;
                    update_win();
                }
            }
        }
    }

    void update_win() {
        for (auto snake : snakes_) {
            if (snake.len_) {
                win_ = 0;
                return;
            }
        }

        win_ = 1;
    }

    void is_snake_falling(const Map& map,
                          const char* snake_map,
                          const Snake& snake,
                          bool* falling,
                          bool* falling_to_death,
                          int* pushed_while_falling) {
        // The space below the snake's head.
        int below = snake.i_ + W;
        *falling = true;
        *falling_to_death = false;

        int pushed_ids = 1 << map_id_to_index(snake.id_);

        for (int j = 0; j < snake.len_; ++j) {
            if (map[below] == '.') {
                *falling = false;
                return;
            }
            if (snake_map[below] &&
                snake_map[below] != snake.id_) {
                int new_pushed_ids = 0;
                if (is_valid_push(map, snake_map, snake.id_,
                                  below - W, W,
                                  &new_pushed_ids)) {
                    pushed_ids |= new_pushed_ids;
                } else {
                    *falling = false;
                    return;
                }
            }

            if (map[below] == '~') {
                *falling_to_death = true;
            }

            below -= Snake::apply_direction(snake.tail(j));
        }

        *pushed_while_falling = pushed_ids;
    }

    void is_gadget_falling(const Map& map,
                           const char* snake_map,
                           int gadget_index,
                           bool* falling,
                           bool* falling_to_death,
                           int* pushed_while_falling) {
        const auto& gadget = map.gadgets_[gadget_index];

        *falling = true;
        *falling_to_death = false;

        int pushed_ids = 1 << (gadget_index + SnakeCount);
        int id = ('0' + gadget_index);

        for (int j = 0; j < gadget.size_; ++j) {
            int at = gadget.i_[j] + gadget_offset_[gadget_index];
            int below = at + W;
            if (map[below] == '.') {
                *falling = false;
                return;
            }
            if (snake_map[below] &&
                snake_map[below] != id) {
                int new_pushed_ids = 0;
                if (is_valid_push(map, snake_map, id, at, W, &new_pushed_ids)) {
                    pushed_ids |= new_pushed_ids;
                } else {
                    *falling = false;
                    return;
                }
            }

            if (map[below] == '~') {
                *falling_to_death = true;
            }
        }

        *pushed_while_falling = pushed_ids;
    }

    bool snake_intersects_exit(const Map& map,
                               const Snake& snake) {
        // Only the head of the snake will trigger an exit
        return snake.i_ == map.exit_;
    }

    bool snake_intersects_water(const Map& map, const Snake& snake) {
        int i = snake.i_;
        for (int j = 0; j < snake.len_; ++j) {
            if (map[i] == '~')
                return true;
            i -= Snake::apply_direction(snake.tail(j));
        }

        return false;
    }

    bool gadget_intersects_water(const Map& map,
                                 int gadget_index) {
        int offset = gadget_offset_[gadget_index];
        if (offset == kGadgetDeleted)
            return false;
        const auto& gadget = map.gadgets_[gadget_index];
        for (int j = 0; j < gadget.size_; ++j) {
            if (map[gadget.i_[j] + offset] == '~')
                return true;
        }

        return false;
    }

    void draw_snakes(const Map& map, char* snake_map,
                     bool draw_path) {
        memset(snake_map, 0, H * W);
        for (auto snake : snakes_) {
            draw_snake(snake_map, snake, draw_path);
        }
        for (int i = 0; i < FruitCount; ++i) {
            if (fruit_active(i)) {
                snake_map[map.fruit_[i]] = 'Q';
            }
        }
        for (int i = 0; i < GadgetCount; ++i) {
            int offset = gadget_offset_[i];
            if (offset != kGadgetDeleted) {
                const auto& gadget = map.gadgets_[i];
                for (int j = 0; j < gadget.size_; ++j) {
                    snake_map[offset + gadget.i_[j]] = '0' + i;
                }
            }
        }
    }

    void draw_snake(char* snake_map, const Snake& snake, bool draw_path) {
        int i = snake.i_;
        for (int j = 0; j < snake.len_; ++j) {
            if (j == 0 || !draw_path) {
                snake_map[i] = snake.id_;
            } else {
                switch (snake.tail(j - 1)) {
                case UP: snake_map[i] = '^'; break;
                case DOWN: snake_map[i] = 'v'; break;
                case LEFT: snake_map[i] = '<'; break;
                case RIGHT: snake_map[i] = '>'; break;
                }
            }
            // ^ (32 * (j == 0)); // Use different case for head
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
    int16_t gadget_offset_[GadgetCount];
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
bool search(St start_state, const Map& map) {
    printf("%ld\n", sizeof(St));
    St null_state;

    // Just in case the starting state is invalid.
    start_state.process_gravity(map);

    // BFS state
    std::deque<St> todo;
    // std::unordered_map<St, St, hash<St>, eq<St>> seen_states;
    google::dense_hash_map<St, St, hash<St>, eq<St>> seen_states;
    seen_states.set_empty_key(null_state);
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
        while (!(win_state == null_state)) {
            win_state.print(map);
            win_state = seen_states[win_state];
            ++moves;
        }
    }

    printf("%ld states, %d moves\n", steps, moves);

    return win;
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

int main() {
    assert(level_00());
    assert(level_01());
    assert(level_02());
    assert(level_03());
    assert(level_04());
    assert(level_05());
    assert(level_06());
    assert(level_07());
    assert(level_08());
    assert(level_09());
    assert(level_10());
    assert(level_11());
    assert(level_12());
    assert(level_13());
    assert(level_14());
    assert(level_15());
    assert(level_16());
    assert(level_17());
    assert(level_18());
    // assert(level_19());
    assert(level_20());
    assert(level_21());
    assert(level_22());

    return 0;
}

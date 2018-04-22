#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <third-party/cityhash/city.h>
#include <unordered_map>
#include <vector>

template<int H, int W, int MaxLen>
class Snake {
public:
    enum Direction {
        UP, RIGHT, DOWN, LEFT,
    };

    Snake() : tail_(0), i_(0), len_(0), id_(0) {
    }

    Snake(int id, int r, int c)
        : tail_(0),
          i_(r * W + c),
          len_(1),
          id_(id) {
        assert(i_ < H * W);
    }

    void grow(Direction dir) {
        i_ += apply_direction(dir);
        ++len_;
        tail_ = (tail_ << 2) | dir;
    }

    void move(Direction dir) {
        i_ += apply_direction(dir);
        tail_ &= ~(0x3 << (len_) * 2);
        tail_ = (tail_ << 2) | dir;
    }

    Direction tail(int i) const {
        return Direction((tail_ >> (i * 2)) & 0x3);
    }

    static int apply_direction(Direction dir) {
        static int deltas[] = { -W, 1, W, -1 };
        return deltas[dir];
    }

    int tail_;
    uint16_t i_;
    uint8_t len_;
    uint8_t id_;
};

template<int H, int W, int FruitCount, int SnakeCount, int SnakeMaxLen>
class State {
public:
    using Snake = typename ::Snake<H, W, SnakeMaxLen>;
    using Direction = typename Snake::Direction;

    State() : map_(NULL) {
    }

    State(const char* map)
        : map_(strdup(map)) {
        assert(strlen(map) == H * W);
    }

    void print() {
        char snake_map[H * W];
        draw_snakes(snake_map);

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
                if (l == exit_) {
                    printf("*");
                } else if (snake_map[l]) {
                    printf("%c", snake_map[l]);
                } else {
                    printf("%c", map_[l]);
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

    int add_fruit(int r, int c, int i) {
        assert(i < FruitCount);
        fruit_[i] = r * W + c;
        return i + 1;
    }

    void set_exit(int r, int c) {
        exit_ = r * W + c;
    }

    void delete_fruit(int i) {
        for (auto& fruit: fruit_) {
            if (fruit == i) {
                fruit = 0;
            }
        }
    }

    void do_valid_moves(std::function<bool(State)> fun) {
        static Direction dirs[] = {
            Snake::UP, Snake::RIGHT, Snake::DOWN, Snake::LEFT,
        };
        char snake_map[H * W];
        draw_snakes(snake_map);
        for (int s = 0; s < SnakeCount; ++s) {
            if (!snakes_[s].len_) {
                continue;
            }
            for (auto dir : dirs) {
                int to = snakes_[s].i_ + Snake::apply_direction(dir);
                if (is_valid_grow(snakes_[s], to)) {
                    State new_state(*this);
                    new_state.snakes_[s].grow(dir);
                    new_state.delete_fruit(to);
                    new_state.process_gravity();
                    if (fun(new_state)) {
                        return;
                    }
                } else if (is_valid_move(snake_map, snakes_[s], to)) {
                    State new_state(*this);
                    new_state.snakes_[s].move(dir);
                    new_state.process_gravity();
                    if (fun(new_state)) {
                        return;
                    }
                }
            }
        }
    }

    bool is_valid_grow(const Snake& snake, int to) {
        for (auto fruit : fruit_) {
            if (fruit == to) {
                return true;
            }
        }

        return false;
    }

    bool is_valid_move(const char* snake_map,
                       const Snake& snake, int to) {
        if (!snake_map[to] &&
            map_[to] == ' ') {
            return true;
        }

        return false;
    }

    void process_gravity() {
        bool again;
        do {
            again = false;
            char snake_map[H * W];
            check_exits();
            draw_snakes(snake_map);
            for (auto& snake : snakes_) {
                if (snake.len_ &&
                    !snake_is_supported(snake_map, snake)) {
                    snake.i_ += W;
                    again = true;
                }
            }
        } while (again);
    }

    void check_exits() {
        for (auto fruit: fruit_) {
            if (fruit) {
                return;
            }
        }

        for (auto& snake : snakes_) {
            if (snake.len_) {
                if (snake_intersects_exit(snake)) {
                    snake.len_ = 0;
                }
            }
        }
    }

    // Consider snake S supported if there is at least one square
    // that's non-empty on the base map or contains a segment of
    // another snake, and which is directly beneath a segment of S.
    //
    // Note: this is incorrect, and just an expedient choice to get
    // started with. Consider something like:
    //
    //     AA
    //    BBA
    //   . AA
    //   .
    //   ....
    //
    // Proper solution that requires two passes + a graph search:
    //
    // - First determine for all snakes if they are supported by
    //   a) the base map, b) other snakes (and which ones?).
    // - Then for each snake try to trace through the support
    //   graph all the way to the ground.
    bool snake_is_supported(const char* snake_map, const Snake& snake) {
        // The space below the snake's head.
        int below = snake.i_ + W;

        for (int j = 0; j < snake.len_; ++j) {
            if ((snake_map[below] != '\0' &&
                 snake_map[below] != snake.id_) ||
                (map_[below] != ' ')) {
                return true;
            }

            below -= Snake::apply_direction(snake.tail(j));
        }

        return false;
    }

    bool snake_intersects_exit(const Snake& snake) {
        int i = snake.i_;

        for (int j = 0; j < snake.len_; ++j) {
            if (i == exit_) {
                return true;
            }

            i -= Snake::apply_direction(snake.tail(j));
        }

        return false;
    }

    void draw_snakes(char* snake_map) {
        memset(snake_map, 0, H * W);
        for (auto snake : snakes_) {
            draw_snake(snake_map, snake);
        }
        for (auto fruit : fruit_) {
            if (fruit) {
                snake_map[fruit] = 'Q';
            }
        }
    }

    void draw_snake(char* snake_map, const Snake& snake) {
        int i = snake.i_;
        for (int j = 0; j < snake.len_; ++j) {
            snake_map[i] = snake.id_;
            // ^ (32 * (j == 0)); // Use different case for head
            i -= Snake::apply_direction(snake.tail(j));
        }
    }

    bool win() {
        for (auto snake : snakes_) {
            if (snake.len_) {
                return false;
            }
        }

        return true;
    }

    char* map_;
    Snake snakes_[SnakeCount];
    uint16_t fruit_[FruitCount];
    uint16_t exit_;
    // FIXME potential padding, might cause hash / eq issues if nonzero
};

template<class T>
struct hash {
    uint64_t operator()(const T& key) const {
        return CityHash64(reinterpret_cast<const char*>(&key), sizeof(key));
    }
};

template<class T>
struct eq {
    bool operator()(const T& a, const T& b) const {
        return memcmp(&a, &b, sizeof(a)) == 0;
    }
};

const char* map =
    "......."
    ".     ."
    ".     ."
    ".     ."
    ".  .. ."
    ".     ."
    ".     ."
    ".......";

int main() {
    using St = State<8, 7, 2, 1, 4>;
    St null_state;
    St start(map);
    printf("%ld\n", sizeof(St));

    start.set_exit(1, 1);
    start.add_fruit(4, 2, 0);
    start.add_fruit(1, 5, 1);

    St::Snake a('a', 1, 2);
    a.grow(St::Snake::RIGHT);
    a.grow(St::Snake::DOWN);
    start.add_snake(a, 0);

    start.process_gravity();

    std::deque<St> todo;
    std::unordered_map<St, St, hash<St>, eq<St>> seen_states;

    todo.push_back(start);
    seen_states[start] = null_state;

    size_t steps = 0;

    St win_state = null_state;
    bool win = false;

    while (!todo.empty() && !win) {
        auto st = todo.front();
        todo.pop_front();

        ++steps;
        if (!(steps & 0xffff)) {
            printf(".");
            fflush(stdout);
        }

        st.do_valid_moves([&st, &todo, &seen_states, &win,
                           &win_state](St new_state) {
                if (seen_states.find(new_state) != seen_states.end()) {
                    return false;
                }

                seen_states[new_state] = st;
                todo.push_back(new_state);

                if (new_state.win()) {
                    win_state = new_state;
                    win = true;
                    return true;
                }
                return false;
            });
    }

    printf("%s: examined %ld states\n",
           win ? "Win" : "No solution",
           steps);

    if (win) {
        while (!eq<St>()(win_state, null_state)) {
            win_state.print();
            win_state = seen_states[win_state];
        }
    }

    // a.move(RIGHT);
    // state.print();
    // a.move(RIGHT);
    // state.print();

    return 0;
}

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <third-party/cityhash/city.h>
#include <unordered_set>
#include <vector>

template<int H, int W, int MaxLen>
class Snake {
public:
    enum Direction {
        UP, RIGHT, DOWN, LEFT,
    };

    Snake() : id_(0), i_(0), len_(0), tail_(0) {
    }

    Snake(int id, int r, int c)
        : id_(id),
          i_(r * W + c),
          len_(1),
          tail_(0) {
        assert(i_ < H * W);
    }

    void grow(Direction dir) {
        ++len_;
        tail_ = (tail_ << 2) | dir;
    }

    void move(Direction dir) {
        i_ += apply_direction(dir);
        tail_ = (tail_ << 2) | dir;
    }

    Direction tail(int i) const {
        return Direction((tail_ >> (i * 2)) & 0x3);
    }

    static int apply_direction(Direction dir) {
        static int deltas[] = { -W, 1, W, -1 };
        return deltas[dir];
    }

    int id_;
    int i_;
    int len_;
    int tail_;
};

template<int H, int W, int SnakeCount, int SnakeMaxLen>
class State {
public:
    using Snake = typename ::Snake<H, W, SnakeMaxLen>;
    using Direction = typename Snake::Direction;

    State(const char* map)
        : map_(strdup(map)) {
        assert(strlen(map) == H * W);
    }

    void print() {
        char snake_map[H * W];
        draw_snakes(snake_map);

        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                printf("%c",
                       snake_map[i * W + j] ^ map_[i * W + j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    int add_snake(const Snake& snake, int i) {
        snakes_[i] = snake;
        return i++;
    }

    void do_valid_moves(std::function<bool(State)> fun) {
        static Direction dirs[] = {
            Snake::UP, Snake::RIGHT, Snake::DOWN, Snake::LEFT,
        };
        char snake_map[H * W];
        draw_snakes(snake_map);
        for (int s = 0; s < SnakeCount; ++s) {
            for (auto dir : dirs) {
                if (is_valid_move(snake_map, snakes_[s], dir)) {
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

    bool is_valid_move(const char* snake_map,
                       const Snake& snake, Direction dir) {
        int i = snake.i_ + Snake::apply_direction(dir);

        if ((snake_map[i] ^ map_[i]) == ' ') {
            return true;
        }

        return false;
    }

    void process_gravity() {
        bool again;
        do {
            again = false;
            char snake_map[H * W];
            draw_snakes(snake_map);
            for (auto& snake : snakes_) {
                if (!snake_is_supported(snake_map, snake)) {
                    snake.i_ += W;
                    again = true;
                }
            }
        } while (again);
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

    void draw_snakes(char* snake_map) {
        memset(snake_map, 0, H * W);
        for (auto snake : snakes_) {
            draw_snake(snake_map, snake);
        }
    }

    void draw_snake(char* snake_map, const Snake& snake) {
        int i = snake.i_;
        for (int j = 0; j < snake.len_; ++j) {
            snake_map[i] = snake.id_;
            i -= Snake::apply_direction(snake.tail(j));
        }
    }

    Snake snakes_[SnakeCount];
    char* map_;
};

template<class T>
struct hash {
    uint64_t operator()(const T& key) const {
        return CityHash64(reinterpret_cast<const char*>(&key), sizeof(key));
    }
};

template<class T>
struct eq {
    uint64_t operator()(const T& a, const T& b) const {
        return memcmp(&a, &b, sizeof(a)) == 0;
    }
};

const char* map =
    "......."
    ".     ."
    ".     ."
    ".  .. ."
    ".     ."
    ".     ."
    ".......";

int main() {
    using St = State<7, 7, 1, 4>;
    St state(map);
    // printf("%ld\n", sizeof(St));

    St::Snake a('a', 2, 3);
    a.grow(St::Snake::RIGHT);
    a.grow(St::Snake::DOWN);
    state.add_snake(a, 0);

    state.print();

    std::deque<St> todo;
    std::unordered_set<St, hash<St>, eq<St>> seen_states;

    todo.push_back(state);
    seen_states.insert(state);

    size_t steps = 0;

    while (!todo.empty()) {
        ++steps;
        if (!(steps & 0xffff)) {
            printf(".");
            fflush(stdout);
        }
        auto st = todo.front();
        todo.pop_front();
        st.do_valid_moves([&todo, &seen_states](St new_state) {
                if (seen_states.find(new_state) != seen_states.end()) {
                    return false;
                }
                seen_states.insert(new_state);
                todo.push_back(new_state);
                return false;
            });
    }

    printf("Done: examined %ld states\n", steps);

    // a.move(RIGHT);
    // state.print();
    // a.move(RIGHT);
    // state.print();

    return 0;
}

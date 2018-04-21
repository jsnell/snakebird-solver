#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <vector>

enum Direction {
    UP, RIGHT, DOWN, LEFT,
    MAX_DIR
};

template<int H, int W>
class Snake {
public:
    Snake(int id, int r, int c)
        : id_(id),
          i_(r * W + c) {
        assert(i_ < H * W);
    }

    void grow(Direction dir) {
        tail_.push_front(dir);
    }

    void move(Direction dir) {
        switch (dir) {
        case UP: i_ -= W; break;
        case RIGHT: ++i_; break;
        case DOWN: i_ += W; break;
        case LEFT: --i_; break;
        default: assert(false);
        }
        tail_.push_front(dir);
        tail_.pop_back();
    }

    int id_;
    int i_;
    std::deque<Direction> tail_;
};

template<int H, int W>
class State {
public:
    using Snake = ::Snake<H, W>;

    State(const char* map)
        : map_(strdup(map)) {
        assert(strlen(map) == H * W);
        snake_map_ = new char[H * W];
    }

    void print() {
        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                printf("%c",
                       snake_map_[i * W + j] ^ map_[i * W + j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    void add_snake(Snake* snake) {
        snakes_.push_back(snake);
        draw_snakes();
    }

    void do_valid_moves(std::function<bool(Snake*, Direction dir)> fun) {
        draw_snakes();
        for (auto snake : snakes_) {
            for (int i = 0; i < MAX_DIR; ++i) {
                Direction dir((Direction) i);
                if (is_valid_move(snake, dir)) {
                    if (fun(snake, dir)) {
                        process_gravity();
                        return;
                    }
                }
            }
        }
        printf("No valid moves\n");
    }

    bool is_valid_move(Snake* snake, Direction dir) {
        int i = snake->i_;

        switch (dir) {
        case UP: i -= W; break;
        case RIGHT: ++i; break;
        case DOWN: i += W; break;
        case LEFT: --i; break;
        default: assert(false);
        }

        if ((snake_map_[i] ^ map_[i]) == ' ') {
            return true;
        }

        return false;
    }

    void process_gravity() {
        bool again;
        do {
            again = false;
            draw_snakes();
            for (auto snake : snakes_) {
                if (!snake_is_supported(snake)) {
                    snake->i_ += W;
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
    bool snake_is_supported(Snake* snake) {
        // The space below the snake's head.
        int below = snake->i_ + W;

        if ((snake_map_[below] != '\0' &&
             snake_map_[below] != snake->id_) ||
            (map_[below] != ' ')) {
            return true;
        }

        for (auto dir : snake->tail_) {
            switch (dir) {
            case DOWN: below -= W; break;
            case LEFT: ++below; break;
            case UP: below += W; break;
            case RIGHT: --below; break;
            default: assert(false);
            }

            if ((snake_map_[below] != '\0' &&
                 snake_map_[below] != snake->id_) ||
                (map_[below] != ' ')) {
                return true;
            }
        }

        return false;
    }

    void draw_snakes() {
        memset(snake_map_, 0, H * W);
        for (auto snake : snakes_) {
            draw_snake(snake);
        }
    }

    void draw_snake(Snake* snake) {
        int i = snake->i_;
        snake_map_[i] = snake->id_;

        for (auto dir : snake->tail_) {
            switch (dir) {
            case DOWN: i -= W; break;
            case LEFT: ++i; break;
            case UP: i += W; break;
            case RIGHT: --i; break;
            default: assert(false);
            }
            snake_map_[i] = snake->id_;
        }
    }

    std::vector<Snake*> snakes_;
    char* map_;
    char* snake_map_;
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
    State<7, 7> state(map);

    Snake<7, 7> a('a', 2, 3);
    a.grow(RIGHT);
    a.grow(DOWN);
    state.add_snake(&a);

    state.print();

    for (int i = 0; i < 100; ++i) {
        state.do_valid_moves([](Snake<7, 7>* snake, Direction dir) {
                snake->move(dir);
                return true;
            });
        state.print();
    }
    // a.move(RIGHT);
    // state.print();
    // a.move(RIGHT);
    // state.print();

    return 0;
}

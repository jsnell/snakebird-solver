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

class Snake {
public:
    Snake(int id, int r, int c)
        : id_(id), r_(r), c_(c) {
    }

    void grow(Direction dir) {
        tail_.push_front(dir);
    }

    void move(Direction dir) {
        switch (dir) {
        case UP: --r_; break;
        case RIGHT: ++c_; break;
        case DOWN: ++r_; break;
        case LEFT: --c_; break;
        default: assert(false);
        }
        tail_.push_front(dir);
        tail_.pop_back();
    }

    int id_;
    int r_;
    int c_;
    std::deque<Direction> tail_;
};

class State {
public:
    State(const char* map, int r, int c)
        : map_(strdup(map)),
          r_(r),
          c_(c) {
        assert(strlen(map) == r * c);
        snake_map_ = new char[r * c];
    }

    void print() {
        for (int i = 0; i < r_; ++i) {
            for (int j = 0; j < c_; ++j) {
                printf("%c",
                       snake_map_[i * c_ + j] ^ map_[i * c_ + j]);
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
        int r = snake->r_;
        int c = snake->c_;

        switch (dir) {
        case UP: --r; break;
        case RIGHT: ++c; break;
        case DOWN: ++r; break;
        case LEFT: --c; break;
        default: assert(false);
        }

        if ((snake_map_[r * c_ + c] ^ map_[r * c_ + c]) == ' ') {
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
                    snake->r_++;
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
        int r = snake->r_ + 1;
        int c = snake->c_;

        if ((snake_map_[r * c_ + c] != '\0' &&
             snake_map_[r * c_ + c] != snake->id_) ||
            (map_[r * c_ + c] != ' ')) {
            return true;
        }

        for (auto dir : snake->tail_) {
            switch (dir) {
            case DOWN: --r; break;
            case LEFT: ++c; break;
            case UP: ++r; break;
            case RIGHT: --c; break;
            default: assert(false);
            }

            if ((snake_map_[r * c_ + c] != '\0' &&
                 snake_map_[r * c_ + c] != snake->id_) ||
                (map_[r * c_ + c] != ' ')) {
                return true;
            }
        }

        return false;
    }

    void draw_snakes() {
        memset(snake_map_, 0, r_ * c_);
        for (auto snake : snakes_) {
            draw_snake(snake);
        }
    }

    void draw_snake(Snake* snake) {
        int r = snake->r_;
        int c = snake->c_;

        snake_map_[r * c_ + c] = snake->id_;

        for (auto dir : snake->tail_) {
            switch (dir) {
            case DOWN: --r; break;
            case LEFT: ++c; break;
            case UP: ++r; break;
            case RIGHT: --c; break;
            default: assert(false);
            }
            snake_map_[r * c_ + c] = snake->id_;
        }
    }

    std::vector<Snake*> snakes_;
    char* map_;
    char* snake_map_;
    int r_;
    int c_;
};

const char* map =
    "......."
    ".     ."
    ".     ."
    ".  .. ."
    ".     ."
    ". *   ."
    ".......";

int main() {
    State state(map, 7, 7);

    Snake a('a', 2, 3);
    a.grow(RIGHT);
    a.grow(DOWN);
    state.add_snake(&a);

    state.print();

    for (int i = 0; i < 10; ++i) {
        state.do_valid_moves([](Snake* snake, Direction dir) {
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

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <vector>

enum Direction {
    UP, RIGHT, DOWN, LEFT
};

class Snake {
public:
    Snake(int r, int c)
        : r_(r), c_(c) {
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
        }
        tail_.push_front(dir);
        tail_.pop_back();
    }

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
        for (auto snake : snakes_) {
            draw_snake(snake);
        }
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
    }

    void draw_snake(Snake* snake) {
        int r = snake->r_;
        int c = snake->c_;

        memset(snake_map_, 0, r_ * c_);
        snake_map_[r * c_ + c] = 'a';

        for (auto dir : snake->tail_) {
            switch (dir) {
            case DOWN: --r; break;
            case LEFT: ++c; break;
            case UP: ++r; break;
            case RIGHT: --c; break;
            }
            snake_map_[r * c_ + c] = 'A';
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
    ". *   ."
    ".......";

int main() {
    State state(map, 6, 7);

    Snake a(2, 3);
    a.grow(RIGHT);
    a.grow(DOWN);
    state.add_snake(&a);

    state.print();
    a.move(RIGHT);
    state.print();
    a.move(RIGHT);
    state.print();

    return 0;
}

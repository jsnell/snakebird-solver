// -*- mode: c++ -*-

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <vector>

#include "bit-packer.h"
#include "compress.h"
#include "file-backed-array.h"
#include "snakebird/snakebird.h"
#include "search.h"

#define EXPECT_EQ(wanted, actual)                                       \
    do {                                                                \
        printf("Running %s\n", #actual);                                \
        auto tmp = actual;                                              \
        if (tmp != wanted) {                                            \
            fprintf(stderr, "Error: expected %s => %d, got %d\n", #actual, wanted, tmp); \
        } \
    } while (0)

template<class St, class Map>
int search(St start_state, const Map& map) {
    // Just in case the starting state is invalid.
    start_state.process_gravity(map, 0);

    class SnakeBirdSearch {
    public:
        static void start_iteration(int depth) {
            printf("depth: %d\n", depth);
        }

        static void trace(const Map& setup, const St& state, int depth) {
            printf("Move %d\n", depth);
            state.print(setup);
        }
    };

    BreadthFirstSearch<St, Map, SnakeBirdSearch> bfs;
    return bfs.search(start_state, map);
}

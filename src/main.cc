#include <algorithm>
#include <cassert>
#include <cmath>
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

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "bit-packer.h"
#include "file-backed-array.h"
#include "game.h"


template<class Seen, class Todo, class T>
void dedup(Seen* seen_states, Todo* todo, T* new_begin, T* new_end) {
    Seen new_seen_states;
    auto new_it = new_begin;
    auto seen_it = seen_states->begin();
    while (true) {
        bool have_new = new_it != new_end;
        bool have_seen = seen_it != seen_states->end();
        if (have_new && have_seen) {
            const auto& new_front = new_it->a;
            const auto& seen_front = seen_it->a;;
            if (new_front < seen_front) {
                todo->push_back(new_front);
                new_seen_states.push_back(*new_it);
                ++new_it;
            } else if (new_front == seen_front) {
                new_seen_states.push_back(*seen_it);
                ++new_it;
                ++seen_it;
            } else {
                new_seen_states.push_back(*seen_it);
                ++seen_it;
            }
        } else if (have_new) {
            todo->push_back(new_it->a);
            new_seen_states.push_back(*new_it);
            ++new_it;
        } else if (have_seen) {
            new_seen_states.push_back(*seen_it);
            ++seen_it;
        } else {
            break;
        }
    }
    // new_states->clear();
    *seen_states = std::move(new_seen_states);
}

template<size_t chunk_bytes = 1000000000, class T, class Cmp>
void sort_in_chunks(T* start, T* end, Cmp cmp) {
    size_t elem_size = sizeof(*start);
    size_t chunk_elems = chunk_bytes / elem_size;
    for (T* s = start; s < end; ) {
        size_t actual_elems = std::min(chunk_elems, (size_t) (end - s));
        T* copy_s = new T[actual_elems];
        T* e = s + actual_elems;
        T* copy_e = copy_s + actual_elems;
        std::copy(s, e, copy_s);
        std::sort(copy_s, copy_e, cmp);
        std::copy(copy_s, copy_e, s);
        s = e;
        delete[] copy_s;
    }
    size_t merge_elems = chunk_elems;
    while (merge_elems < end - start) {
        for (T* s = start; s + merge_elems < end; ) {
            T* m = s + merge_elems;
            T* e = m + std::min(merge_elems, (size_t) (end - m));
            std::inplace_merge(s, m, e, cmp);
            s = e;
        }
        merge_elems *= 2;
    }
}

template<class St>
class OutputState {
public:
    explicit OutputState(int i) {
    }

    OutputState(const OutputState& other) = delete;
    void operator=(const OutputState& other) = delete;

    OutputState(OutputState&& other)
        :  seen_states_(std::move(other.seen_states_)),
           new_states_(std::move(other.new_states_)) {
    }

    void insert(St st) {
        new_states_.push_back(st);
    }

    void flush() {
        seen_states_.freeze();
        new_states_.freeze();
    }

    void start_iteration() {
        new_states_.reset();
    }

    file_backed_mmap_array<St>& seen_states_mmap() {
        return seen_states_;
    }

    file_backed_mmap_array<St>& new_states_mmap() {
        return new_states_;
    }

    file_backed_mmap_array<St> seen_states_;
    file_backed_mmap_array<St> new_states_;
};

template<class St, class Map>
int search(St start_state, const Map& map) {
    using Packed = typename St::Packed;
    St null_state;

    // Just in case the starting state is invalid.
    start_state.process_gravity(map, 0);

    struct st_pair {
        st_pair() {
        }

        st_pair(const St& a, uint8_t depth) : a(a), depth(depth) {
        }

        bool operator<(const st_pair& other) const {
            return a < other.a;
        }
        bool operator==(const st_pair& other) const {
            return a == other.a;
        }

        Packed a;
        uint8_t depth = 0;
    };
    printf("bits=%ld packed_bytes=%ld\n", St::packed_width(),
           sizeof(Packed));

    // BFS state
    std::vector<Packed> todo;
    size_t steps = 0;
    St win_state = null_state;
    bool win = false;

    {
        Packed p(start_state);
        St s(p);
        s.print(map);
    }

    std::vector<OutputState<st_pair>> outputs;
    const int kShards = (1 << 4);
    for (int i = 0; i < kShards; ++i) {
        outputs.emplace_back(OutputState<st_pair>(i));
    }

    outputs[0].insert(st_pair(start_state, 0));

    size_t total_states = 0;
    size_t depth = 0;
    while (1) {
        // Empty the todo list
        todo.clear();

        size_t seen_states_size = 0;
        size_t new_states_size = 0;
        for (auto& output : outputs) {
            output.flush();
            file_backed_mmap_array<st_pair>& new_states =
                output.new_states_mmap();
            file_backed_mmap_array<st_pair>& seen_states =
                output.seen_states_mmap();
            // Sort and dedup just new_states
            auto cmp = [](const st_pair& a, const st_pair &b) { return a < b;};
            sort_in_chunks(new_states.begin(), new_states.end(), cmp);

            seen_states_size += seen_states.size();
            new_states_size += new_states.size();
            total_states += new_states.size();
            auto new_end = std::unique(new_states.begin(), new_states.end());

            // Build a new todo list from the entries in new_states not
            // contained in seen_states.
            dedup(&seen_states, &todo, new_states.begin(), new_end);
            output.start_iteration();
        }

        printf("depth: %ld unique:%ld new:%ld (total: %ld, delta %ld)\n",
               depth++,
               seen_states_size, todo.size(),
               total_states, new_states_size);

        if (win || todo.empty()) {
            break;
        }

        for (auto packed : todo) {
            St st(packed);

            if (!(++steps & 0xffff)) {
                printf(".");
                fflush(stdout);
            }

            st.do_valid_moves(map,
                              [&depth, &outputs, &win, &map,
                               &win_state](St new_state,
                                           int si,
                                           Direction dir) {
                                  new_state.canonicalize(map);
                                  st_pair pair(new_state, depth);
                                  auto hash = CityHash64((char*) pair.a.p_.bytes_,
                                                         sizeof(pair.a.p_.bytes_));
                                  outputs[hash & (kShards - 1)].insert(pair);
                                  // new_states.push_back(
                                  //     st_pair(new_state, depth));
                                  if (new_state.win()) {
                                      win_state = new_state;
                                      win = true;
                                      return true;
                                  }
                                  return false;
                              });
        }
    }

    printf("%s\n",
           win ? "Win" : "No solution");

    if (win) {
        win_state.print(map);

        Packed target(win_state);

        std::vector<OutputState<st_pair>> outputs_by_depth;
        for (int i = 0; i < depth; ++i) {
            outputs_by_depth.emplace_back(OutputState<st_pair>(i));
        }
        while (!outputs.empty()) {
            auto& output = outputs.back();
            output.flush();
            file_backed_mmap_array<st_pair>& seen_states =
                output.seen_states_mmap();
            for (const auto& st : seen_states) {
                outputs_by_depth[st.depth].insert(st);
            }
            outputs.pop_back();
        }
        for (auto& output : outputs_by_depth) {
            output.flush();
            file_backed_mmap_array<st_pair>& new_states =
                output.new_states_mmap();
            auto cmp = [](const st_pair& a, const st_pair &b) {
                return a < b;
            };
            sort_in_chunks(new_states.begin(), new_states.end(), cmp);
        }

        for (int d = depth - 1; d >= 0; --d) {
            file_backed_mmap_array<st_pair>& seen_states =
                outputs_by_depth[d].new_states_mmap();
            for (const auto& pair : seen_states) {
                St st(pair.a);
                if (st.do_valid_moves(map,
                                      [&target, &map](St new_state,
                                                      int si,
                                                      Direction dir) {
                                          new_state.canonicalize(map);
                                          Packed p(new_state);
                                          if (p == target) {
                                              return true;
                                          }
                                          return false;
                                      })) {
                    st.print(map);
                    target = st;
                    break;
                }
            }
        }
    }

    printf("%ld states, %ld moves\n", steps, depth);

    return win ? depth - 1 : 0;
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
#include "levelstar2.h"
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
    EXPECT_EQ(45, level_29());
    EXPECT_EQ(15, level_30());
    EXPECT_EQ(8, level_31());
    EXPECT_EQ(21, level_32());
    EXPECT_EQ(42, level_33());
    EXPECT_EQ(17, level_34());
    EXPECT_EQ(29, level_35());
    EXPECT_EQ(29, level_36());
    EXPECT_EQ(16, level_37());
    EXPECT_EQ(28, level_38());
    EXPECT_EQ(53, level_39());
    EXPECT_EQ(51, level_40());
    EXPECT_EQ(34, level_41());
    EXPECT_EQ(42, level_42());
    EXPECT_EQ(36, level_43());
    EXPECT_EQ(36, level_44());
    EXPECT_EQ(77, level_45());
    EXPECT_EQ(75, level_star1());     // 9GB, 30 minutes
    EXPECT_EQ(60, level_star2());
    EXPECT_EQ(63, level_star3());   // 2.2GB
    EXPECT_EQ(44, level_star4());
    EXPECT_EQ(69, level_star5());
    EXPECT_EQ(75, level_star6());   // 12GB, 40 minutes


    return 0;
}

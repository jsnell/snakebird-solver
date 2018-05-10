#include <algorithm>
#include <cassert>
#include <chrono>
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
    if (seen_states->size()) {
        auto it = new_begin;
        auto prev = *seen_states->begin();
        for (const auto& st : *seen_states) {
            if (st < prev) {
                it = std::lower_bound(new_begin, new_end, st);
            }
            while (it != new_end && *it < st) {
                ++it;
            }
            if (st == *it) {
                it->parent_hash = 255;
            }
            prev = st;
        }
    }

    seen_states->thaw();

    for (T* it = new_begin; it != new_end; ++it) {
        if (it->parent_hash != 255) {
            seen_states->push_back(*it);
            todo->push_back(it->a);
        }
    }
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

template<class St,
         // 200M
         size_t kMemoryTargetBytes=200000000>
class OutputState {
public:
    explicit OutputState(int i) {
    }

    OutputState(const OutputState& other) = delete;
    void operator=(const OutputState& other) = delete;

    OutputState(OutputState&& other)
        :  seen_states_(std::move(other.seen_states_)),
           new_states_(std::move(other.new_states_)) {
        seen_states_round_start_.push_back(0);
    }

    void insert(St st) {
        new_states_.push_back(st);
    }

    void flush() {
        seen_states_round_start_.push_back(seen_states_.size());
        seen_states_.freeze();
        new_states_.freeze();
    }

    void start_iteration() {
        new_states_.reset();
        seen_states_round_end_.push_back(seen_states_.size());
    }

    St* round_begin(int round) {
        return seen_states_.begin() + seen_states_round_start_[round];
    }

    St* round_end(int round) {
        return seen_states_.begin() + seen_states_round_end_[round];
    }

    using SeenStates = file_backed_mmap_array<St, kMemoryTargetBytes/2>;
    using NewStates = file_backed_mmap_array<St, kMemoryTargetBytes/2>;

    SeenStates& seen_states_mmap() {
        return seen_states_;
    }

    NewStates& new_states_mmap() {
        return new_states_;
    }


    SeenStates seen_states_;
    NewStates new_states_;
    std::vector<size_t> seen_states_round_start_;
    std::vector<size_t> seen_states_round_end_;
};

template<class Clock=typename std::chrono::high_resolution_clock>
struct MeasureTime {
    MeasureTime(double* target)
        : target_(target),
          start_(Clock::now()) {

    }

    ~MeasureTime() {
        auto end = Clock::now();
        // What a marvelous API.
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start_);
        *target_ += duration.count();
    }

    double* target_;
    typename Clock::time_point start_;
};

template<class T, size_t SZ>
int reshard(std::vector<OutputState<T, SZ>>* outputs,
             int new_shards) {
    assert(outputs->empty());
    assert(!(new_shards & (new_shards - 1)));

    for (int i = 0; i < new_shards; ++i) {
        outputs->emplace_back(OutputState<T, SZ>(i));
    }

    return new_shards - 1;
}

template<class St, class Map>
int search(St start_state, const Map& map) {
    using Packed = typename St::Packed;
    St null_state;

    // Just in case the starting state is invalid.
    start_state.process_gravity(map, 0);

    struct st_pair {
        st_pair() {
        }

        st_pair(const St& a, uint8_t parent_hash)
            : a(a), parent_hash(parent_hash) {
        }

        bool operator<(const st_pair& other) const {
            return a < other.a;
        }
        bool operator==(const st_pair& other) const {
            return a == other.a;
        }

        Packed a;
        uint8_t parent_hash = 0;
    };

    // BFS state
    std::vector<Packed> todo;
    size_t steps = 0;
    st_pair win_state = st_pair(null_state, 0);
    bool win = false;

    const size_t kTargetMemoryBytes = 3UL * 1024 * 1024 * 1024;
    const int kShards = 16;
    std::vector<OutputState<st_pair, kTargetMemoryBytes / kShards>> outputs;
    int shard_mask = reshard<st_pair>(&outputs, kShards);

    printf("bits=%ld packed_bytes=%ld output_state=%ldMB\n",
           St::packed_width(),
           sizeof(Packed),
           kTargetMemoryBytes / kShards / 1000 / 1000);

    {
        auto pair = st_pair(start_state, 0);
        auto hash = CityHash64((char*) pair.a.p_.bytes_,
                               sizeof(pair.a.p_.bytes_));
        pair.parent_hash = hash & shard_mask;
        outputs[hash & shard_mask].insert(pair);
        St(pair.a).print(map);
    }

    size_t total_states = 0;
    size_t depth = 0;
    double dedup_s = 0, search_s = 0, print_s = 0;
    while (1) {
        // Empty the todo list
        todo.clear();

        size_t seen_states_size = 0;
        size_t new_states_size = 0;
        for (auto& output : outputs) {
            MeasureTime<> timer(&dedup_s);
            output.flush();
            auto& new_states = output.new_states_mmap();
            auto& seen_states = output.seen_states_mmap();

            total_states += new_states.size();
            new_states_size += new_states.size();

            // Sort and dedup just new_states
            auto cmp = [](const st_pair& a, const st_pair &b) { return a < b;};
            sort_in_chunks(new_states.begin(), new_states.end(), cmp);

            auto new_end = std::unique(new_states.begin(), new_states.end());

            // Build a new todo list from the entries in new_states not
            // contained in seen_states.
            dedup(&seen_states, &todo, new_states.begin(), new_end);

            seen_states_size += seen_states.size();
            output.start_iteration();
        }

        printf("depth: %ld unique:%ld, delta %ld (total: %ld, delta %ld)\n",
               depth++,
               seen_states_size, todo.size(),
               total_states, new_states_size);
        printf("timing: dedup: %lfs search: %lfs = total: %lfs\n",
               dedup_s, search_s, dedup_s + search_s);

        if (win || todo.empty()) {
            break;
        }

        MeasureTime<> timer(&search_s);
        for (auto packed : todo) {
            St st(packed);

            if (!(++steps & 0xffff)) {
                printf(".");
                fflush(stdout);
            }
            auto parent_hash = CityHash64((char*) packed.p_.bytes_,
                                          sizeof(packed.p_.bytes_));

            st.do_valid_moves(map,
                              [&depth, &outputs, &win, &map, &parent_hash,
                               &shard_mask,
                               &win_state](St new_state,
                                           int si,
                                           Direction dir) {
                                  new_state.canonicalize(map);
                                  st_pair pair(new_state, 0);
                                  auto hash = CityHash64((char*) pair.a.p_.bytes_,
                                                         sizeof(pair.a.p_.bytes_));
                                  pair.parent_hash = parent_hash;
                                  outputs[hash & shard_mask].insert(pair);
                                  // new_states.push_back(
                                  //     st_pair(new_state, depth));
                                  if (new_state.win()) {
                                      win_state = pair;
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
        MeasureTime<> timer(&print_s);
        St(win_state.a).print(map);

        st_pair target = win_state;

        for (auto& output : outputs) {
            output.flush();
        }

        for (int i = depth - 1; i > 0; --i) {
            auto h = target.parent_hash & shard_mask;
            const auto begin = outputs[h].round_begin(i);
            const auto end = outputs[h].round_end(i);
            printf("Move %d\n", i);
            for (auto it = begin; it != end + 10; ++it) {
                St st(it->a);
                if (st.do_valid_moves(map,
                                      [&target, &map](St new_state,
                                                      int si,
                                                      Direction dir) {
                                          new_state.canonicalize(map);
                                          Packed p(new_state);
                                          if (p == target.a) {
                                              return true;
                                          }
                                          return false;
                                      })) {
                    st.print(map);
                    target = *it;
                    break;
                }
            }
        }
    }

    printf("%ld states, %ld moves\n", steps, depth);
    printf("timing: dedup: %lfs search: %lfs print: %lfs= total: %lfs\n",
           dedup_s, search_s, print_s, dedup_s + search_s + print_s);

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
#include "levelvoid.h"

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
    EXPECT_EQ(75, level_star1());   // 5GB, 20 minutes
    EXPECT_EQ(60, level_star2());
    EXPECT_EQ(63, level_star3());   // 2.2GB
    EXPECT_EQ(44, level_star4());
    EXPECT_EQ(69, level_star5());
    EXPECT_EQ(90, level_star6());   // 40GB, 10 hours
    EXPECT_EQ(52, level_void());


    return 0;
}

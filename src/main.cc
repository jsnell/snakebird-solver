#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <third-party/cityhash/city.h>
#include <vector>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "bit-packer.h"
#include "file-backed-array.h"
#include "game.h"


template<class Key, class Value>
class Pair {
public:
    Pair() {
    }

    Pair(const Key& key, Value value)
        : key_(key), value_(value) {
    }

    bool operator<(const Key& key) const {
        return key_ < key;
    }
    bool operator==(const Key& key) const {
        return key_ == key;
    }

    bool operator<(const Pair& other) const {
        return key_ < other.key_;
    }
    bool operator==(const Pair& other) const {
        return key_ == other.key_;
    }

    uint64_t hash() const {
        // FIXME
        return CityHash64((char*) key_.p_.bytes_,
                          sizeof(key_.p_.bytes_));
    }

    Key key_;
    Value value_;
};

template<int Length>
uint8_t* unpack_key_from_bytes(uint8_t* in,
                               uint8_t prev[Length],
                               uint8_t output[Length]) {
    uint8_t n = *in++;
    for (int i = 0; i < n; ++i, ++in) {
        output[i] = *in ^ prev[i];
    }

    return in;
}

template<class Keys, class Values, class Todo, class K, class V>
void dedup(Keys* seen_keys, Values* seen_values, Todo* todo,
           Pair<K, V>* new_begin, Pair<K, V>* new_end) {
    std::vector<bool> discard(new_end - new_begin);

    for (int run = 0; run < seen_keys->runs(); ++run) {
        auto runinfo = seen_keys->run(run);
        auto keyit = seen_keys->begin() + runinfo.first;
        auto end = seen_keys->begin() + runinfo.second;
        // auto it = std::lower_bound(new_begin, new_end, key);
        auto it = new_begin;
        K prev;
        while (keyit < end) {
            K key = prev;
            keyit = unpack_key_from_bytes<sizeof(key.p_.bytes_)>
                (keyit, prev.p_.bytes_, key.p_.bytes_);
            while (it != new_end && it->key_ < key) {
                ++it;
            }
            if (key == it->key_) {
                discard[it - new_begin] = true;
            }
            prev = key;
        }
    }

    seen_keys->thaw();
    seen_values->thaw();

    seen_keys->start_run();
    seen_values->start_run();
    K prev;
    for (int i = 0; i < discard.size(); ++i) {
        if (!discard[i]) {
            // seen_keys->push_back(new_begin[i].key_);
            const auto& key = new_begin[i].key_;
            K out;
            int n = 0;
            for (int j = 0; j < sizeof(key.p_.bytes_); ++j) {
                uint8_t diff = prev.p_.bytes_[j] ^ key.p_.bytes_[j];
                out.p_.bytes_[j] = diff;
                if (diff) {
                    n = j + 1;
                }
            }
            seen_keys->push_back(n);
            for (int j = 0; j < n; ++j) {
                seen_keys->push_back(out.p_.bytes_[j]);
            }

            prev = key;
            seen_values->push_back(new_begin[i].value_);
            todo->push_back(new_begin[i].key_);
        }
    }
    seen_keys->end_run();
    seen_values->end_run();
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

template<class Key, class Value,
         // 200M
         size_t kMemoryTargetBytes=200000000>
class OutputState {
public:
    explicit OutputState(int i) {
    }

    OutputState(const OutputState& other) = delete;
    void operator=(const OutputState& other) = delete;

    OutputState(OutputState&& other)
        : keys_(std::move(other.keys_)),
          values_(std::move(other.values_)),
          new_pairs_(std::move(other.new_pairs_)) {
    }

    void insert(const Pair<Key, Value>& pair) {
        new_pairs_.push_back(pair);
    }

    void flush() {
        keys_.flush();
        values_.flush();
        new_pairs_.flush();
    }

    void map() {
        keys_.freeze();
        values_.freeze();
        new_pairs_.snapshot();
    }

    void start_iteration() {
        new_pairs_.reset();
    }

    const static size_t kInMemoryElems = kMemoryTargetBytes / (sizeof(Key) + sizeof(Value)) / 2;
    // using Keys = file_backed_mmap_array<Key, kInMemoryElems>;
    using Keys = file_backed_mmap_array<uint8_t, kInMemoryElems>;
    using Values = file_backed_mmap_array<Value, kInMemoryElems>;
    using NewPairs = file_backed_mmap_array<Pair<Key, Value>, kInMemoryElems>;

    Keys& keys() { return keys_; }
    Values& values() { return values_; }
    NewPairs& new_pairs() { return new_pairs_; }

    Keys keys_;
    Values values_;
    NewPairs new_pairs_;
};

template<class OutputState>
int reshard(std::vector<OutputState>* outputs,
             int new_shards) {
    assert(outputs->empty());
    assert(!(new_shards & (new_shards - 1)));

    for (int i = 0; i < new_shards; ++i) {
        outputs->emplace_back(OutputState(i));
    }

    return new_shards - 1;
}

template<class St, class Map>
int search(St start_state, const Map& map) {
    const size_t kTargetMemoryBytes = 2UL * 1024 * 1024 * 1024;
    const int kShards = 16;

    using Packed = typename St::Packed;
    using st_pair = Pair<Packed, uint8_t>;
    using OutputState = OutputState<Packed,
                                    uint8_t,
                                    kTargetMemoryBytes / kShards>;

    // Just in case the starting state is invalid.
    start_state.process_gravity(map, 0);

    St null_state;

    // BFS state
    std::vector<Packed> todo;
    size_t steps = 0;
    st_pair win_state = st_pair(null_state, 0);
    bool win = false;

    std::vector<OutputState> outputs;
    int shard_mask = reshard<>(&outputs, kShards);

    printf("bits=%ld packed_bytes=%ld output_state=%ldMB\n",
           St::packed_width(),
           sizeof(Packed),
           kTargetMemoryBytes / kShards / 1000 / 1000);

    {
        auto pair = st_pair(start_state, 0xfe);
        auto hash = pair.hash();
        outputs[hash & shard_mask].insert(pair);
        St(pair.key_).print(map);
    }

    size_t total_states = 0;
    size_t depth = 0;
    double dedup_flush_s = 0, dedup_sort_s = 0, dedup_merge_s = 0,
        search_s = 0, print_s = 0;
    while (1) {
        // Empty the todo list
        todo.clear();

        size_t seen_states_size = 0;
        size_t new_states_size = 0;
        for (auto& output : outputs) {
            MeasureTime<> timer(&dedup_flush_s);
            output.flush();
        }

        for (int i = 0; i < outputs.size(); ++i) {
            auto& output = outputs[i];
            {
                MeasureTime<> timer(&dedup_flush_s);
                output.map();
            }

            auto& new_states = output.new_pairs();
            auto& seen_keys = output.keys();
            auto& seen_values = output.values();

            total_states += new_states.size();
            new_states_size += new_states.size();

            st_pair* new_end = NULL;
            {
                // Sort and dedup just new_states
                MeasureTime<> timer(&dedup_sort_s);
                auto cmp = [](const st_pair& a, const st_pair &b) { return a < b;};
                sort_in_chunks(new_states.begin(), new_states.end(), cmp);

                new_end = std::unique(new_states.begin(), new_states.end());
            }

            // Build a new todo list from the entries in new_states not
            // contained in seen_states.
            MeasureTime<> timer(&dedup_merge_s);
            dedup(&seen_keys, &seen_values,
                  &todo, new_states.begin(), new_end);
            seen_states_size += seen_values.size();

            output.start_iteration();
        }

        printf("depth: %ld unique:%ld, delta %ld (total: %ld, delta %ld)\n",
               depth++,
               seen_states_size, todo.size(),
               total_states, new_states_size);
        printf("timing: dedup: %lfs+%lfs+%lfs search: %lfs = total: %lfs\n",
               dedup_flush_s, dedup_sort_s, dedup_merge_s, search_s,
               dedup_flush_s + dedup_sort_s + dedup_merge_s + search_s);

        if (win || todo.empty()) {
            break;
        }

        MeasureTime<> timer(&search_s);
        for (auto packed : todo) {
            St st(packed);

            if (!(++steps & 0x7ffff)) {
                printf(".");
                fflush(stdout);
            }
            auto parent_hash = st_pair(packed, 0).hash();

            st.do_valid_moves(map,
                              [&depth, &outputs, &win, &map, &parent_hash,
                               &shard_mask,
                               &win_state](St new_state,
                                           int si,
                                           Direction dir) {
                                  new_state.canonicalize(map);
                                  st_pair pair(new_state,
                                               parent_hash & 0xff);

                                  auto hash = pair.hash();
                                  outputs[hash & shard_mask].insert(pair);
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
        St(win_state.key_).print(map);

        st_pair target = win_state;

        for (auto& output : outputs) {
            output.flush();
            output.map();
        }

        for (int i = depth - 1; i > 0; --i) {
            auto h = target.value_ & shard_mask;
            auto& seen_keys = outputs[h].keys();
            auto& seen_values = outputs[h].values();
            auto runinfo = seen_keys.run(i - 1);
            const auto begin = &seen_keys[runinfo.first];
            const auto end = &seen_keys[runinfo.second];
            printf("Move %d\n", i);
            Packed prev;
            auto it = begin;
            for (int j = 0; it != end; ++j) {
                Packed key = prev;
                it = unpack_key_from_bytes<sizeof(key.p_.bytes_)>(
                    it, prev.p_.bytes_, key.p_.bytes_);
                prev = key;

                st_pair current(key, 0);
                if ((current.hash() & 0xff) != (target.value_ & 0xff)) {
                    continue;
                }
                St st(key);
                if (st.do_valid_moves(map,
                                      [&target, &map](St new_state,
                                                      int si,
                                                      Direction dir) {
                                          new_state.canonicalize(map);
                                          Packed p(new_state);
                                          if (p == target.key_) {
                                              return true;
                                          }
                                          return false;
                                      })) {
                    const auto& value = seen_values[seen_values.run(i - 1).first + j];
                    target = st_pair(key, value);
                    st.print(map);
                    break;
                }
            }
        }
    }

    printf("%ld states, %ld moves\n", steps, depth);
    printf("timing: dedup: %lfs search: %lfs print: %lfs= total: %lfs\n",
           dedup_sort_s + dedup_merge_s, search_s, print_s,
           dedup_sort_s + dedup_merge_s + search_s + print_s);

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
        printf("Running %s\n", #actual);                                \
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

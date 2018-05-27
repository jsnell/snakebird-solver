// -*- mode: c++ -*-

#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>
#include <vector>

#include "compress.h"
#include "file-backed-array.h"

template<class State, class Setup>
struct BFSPolicy {
    static void start_iteration(int depth) {
    }

    static void trace(const Setup& setup, const State& state, int depth) {
    }
};

template<class State, class Setup,
         class Policy = BFSPolicy<State, Setup>,
         class PackedState = typename State::Packed>
class BreadthFirstSearch {
public:
    using Key = PackedState;
    using Value = uint8_t;
    using st_pair = std::pair<Key, Value>;

    using Keys = file_backed_mmap_array<uint8_t>;
    using Values = file_backed_mmap_array<uint8_t>;
    using NewStates = std::vector<st_pair>;
    using KeyRun = Keys::Run;

    using KeyStream = StructureDeltaDecompressorStream<Key>;
    using ValueStream = PointerStream<Value>;

    int search(State start_state, const Setup& setup) {
        State null_state;

        // BFS state
        Keys seen_keys;
        Values seen_values;
        Keys new_keys;
        Values new_values;
        NewStates new_states;
        bool win = false;
        st_pair win_state { null_state, 0 };

        Keys compacted_seen_keys;
        size_t uncompacted_runs = 0;
        size_t uncompacted_states = 0;

        new_states.emplace_back(start_state, 0);

        seen_keys.freeze();
        compacted_seen_keys.freeze();

        size_t new_state_count = 0;
        for (int iter = 0; ; ++iter) {
            Policy::start_iteration(iter);

            {
                new_state_count += new_states.size();
                pack_pairs(&new_states, &new_keys, &new_values);
                new_keys.freeze();
                new_values.freeze();
            }

            printf("  new states: %ld\n", new_state_count);
            new_state_count = 0;
            fflush(stdout);

            {
                std::vector<KeyRun> runs = compacted_seen_keys.runs();
                std::vector<KeyRun> uncompacted = seen_keys.runs();
                runs.insert(runs.end(),
                            uncompacted.end() - uncompacted_runs,
                            uncompacted.end());
                int uniq = dedup(&seen_keys, &seen_values, runs,
                                 new_keys, new_values);
                uncompacted_states += uniq;
                uncompacted_runs++;
                printf("  new unique: %d\n", uniq);
                new_keys.reset();
                new_values.reset();
            }

            if (win) {
                break;
            }

            auto todo = seen_keys.run(seen_keys.run_count() - 1);

            if (todo.first == todo.second) {
                // We haven't won, and have no moves to process.
                return 0;
            }

            if (uncompacted_runs >= 8 &&
                uncompacted_states >= 10000) {
                std::vector<KeyRun> to_compact = seen_keys.runs();
                to_compact.erase(to_compact.begin(),
                                 to_compact.end() - uncompacted_runs);
                compact_runs(&compacted_seen_keys, to_compact);
                printf("Compacted %ld runs with %ld states, "
                       "orig bytes: %ld new bytes: %ld\n",
                       uncompacted_runs,
                       uncompacted_states,
                       seen_keys.size(),
                       compacted_seen_keys.size());
                uncompacted_runs = 0;
                uncompacted_states = 0;
            }

            StructureDeltaDecompressorStream<Key> stream(todo.first,
                                                         todo.second);

            while (stream.next()) {
                State st(stream.value());
                auto parent_hash = stream.value().hash();

                st.do_valid_moves(setup,
                                  [&new_states, &parent_hash, &win_state,
                                   &win, &setup]
                                  (State new_state) {
                                      new_state.canonicalize(setup);
                                      st_pair pair(new_state,
                                                   parent_hash & 0xff);
                                      new_states.push_back(pair);
                                      if (new_state.win()) {
                                          win_state = pair;
                                          win = true;
                                          return true;
                                      }
                                      return false;
                                  });
                if (new_states.size() > 100000000) {
                    new_state_count += new_states.size();
                    pack_pairs(&new_states, &new_keys, &new_values);
                }
            }
        }

        seen_values.freeze();
        return trace_solution_path(setup, seen_keys, seen_values, win_state);
    }


private:
    int trace_solution_path(const Setup& setup,
                            const Keys& seen_keys,
                            const Values& seen_values,
                            const st_pair win_state) {
        st_pair target = win_state;

        int depth = seen_keys.run_count();

        for (int i = depth - 1; i > 0; --i) {
            Policy::trace(setup, State(target.first), i);

            auto runinfo = seen_keys.run(i - 1);
            StructureDeltaDecompressorStream<Key> stream(runinfo.first,
                                                         runinfo.second);

            bool found_next = false;
            for (int j = 0; stream.next(); ++j) {
                const auto& key = stream.value();
                st_pair current(key, 0);

                if ((key.hash() & 0xff) != (target.second & 0xff)) {
                    continue;
                }

                State st(key);
                if (st.do_valid_moves(setup,
                                      [&target, &setup](State new_state) {
                                          new_state.canonicalize(setup);
                                          Key p(new_state);
                                          if (p == target.first) {
                                              return true;
                                          }
                                          return false;
                                      })) {
                    const auto& value = seen_values.run(i - 1).first[j];
                    target = st_pair(key, value);
                    found_next = true;
                    break;
                }
            }
            assert(found_next);
        }
        Policy::trace(setup, State(target.first), 0);

        return depth - 1;
    }

    void pack_pairs(NewStates* new_states, Keys* new_keys,
                    Values* new_values) {
        std::sort(new_states->begin(), new_states->end(),
                  [] (const st_pair& a, const st_pair& b) {
                      return a.first < b.first;
                  });
        Key prev;

        new_keys->start_run();
        new_values->start_run();
        ByteArrayDeltaCompressor<sizeof(Key::p_.bytes_),
                                 false,
                                 Keys> compress { new_keys };
        for (const auto& pair : *new_states) {
            if (pair.first == prev) {
                continue;
            }

            compress.pack(pair.first.p_.bytes_);
            new_values->push_back(pair.second);
            prev = pair.first;
        }
        new_keys->end_run();
        new_values->end_run();

        new_states->clear();
    }


    template<class Stream, class Interleaver>
    void add_stream_runs(Interleaver* combiner,
                         const std::vector<KeyRun>& runs) {
        for (auto run : runs) {
            if (run.first != run.second) {
                auto stream = new Stream(run.first, run.second);
                combiner->add_stream(stream);
            }
        }
    }

    size_t dedup(Keys* seen_keys, Values* seen_values,
                 const std::vector<KeyRun>& seen_keys_runs,
                 const Keys& new_keys, const Values &new_values) {
        std::vector<bool> discard(new_values.size());

        using PairStream = StreamPairer<Key, Value, KeyStream, ValueStream>;

        if (new_keys.size()) {
            SortedStreamInterleaver<Key, KeyStream> seen_keys_stream;
            SortedStreamInterleaver<Key, KeyStream> new_keys_stream;
            add_stream_runs<KeyStream>(&seen_keys_stream,
                                       seen_keys_runs);
            add_stream_runs<KeyStream>(&new_keys_stream, new_keys.runs());

            new_keys_stream.next();

            size_t i = 0;
            while (seen_keys_stream.next()) {
                while (new_keys_stream.value() < seen_keys_stream.value()) {
                    if (!new_keys_stream.next()) {
                        goto done;
                    }
                    ++i;
                }
                if (new_keys_stream.value() == seen_keys_stream.value()) {
                    discard[i] = true;
                }
            }
        done:
            ;
        }

        ByteArrayDeltaCompressor<sizeof(Key::p_.bytes_),
                                 false,
                                 Keys> compress { seen_keys };

        size_t count = 0;
        seen_keys->thaw();
        seen_keys->start_run();
        seen_values->start_run();


        SortedStreamInterleaver<typename PairStream::Pair, PairStream>
            new_state_stream;
        for (int run = 0; run < new_keys.run_count(); ++run) {
            auto keyinfo = new_keys.run(run);
            auto valinfo = new_values.run(run);
            if (keyinfo.first != keyinfo.second) {
                auto keystream =
                    new KeyStream(keyinfo.first, keyinfo.second);
                auto valstream =
                    new ValueStream(valinfo.first, valinfo.second);
                new_state_stream.add_stream(new PairStream(keystream,
                                                           valstream));
            }
        }

        for (int i = 0; new_state_stream.next(); ++i) {
            if (!discard[i]) {
                ++count;
                compress.pack(new_state_stream.value().first.p_.bytes_);
                seen_values->push_back(new_state_stream.value().second);
            }
        }

        compress.write();
        seen_keys->freeze();
        seen_keys->end_run();
        seen_values->end_run();

        return count;
    }

    size_t compact_runs(Keys* output,
                        const std::vector<KeyRun>& runs) {
        SortedStreamInterleaver<Key, KeyStream> stream;
        add_stream_runs<KeyStream>(&stream, runs);
        output->thaw();
        output->start_run();

        ByteArrayDeltaCompressor<sizeof(Key::p_.bytes_),
                                 false,
                                 Keys> compress { output };

        size_t count = 0;
        for (; stream.next(); ++count) {
            compress.pack(stream.value().p_.bytes_);
        }

        compress.write();

        output->end_run();
        output->freeze();
        return count;
    }
};

#endif

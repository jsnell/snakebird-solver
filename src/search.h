// -*- mode: c++ -*-
//
// A breadth-first search optimized for secondary storage.
//
// This is not exactly a textbook implementation of a BFS. Instead
// checking each output state against a hash table (or similar
// structure) of previously seen states, this code processes the
// whole depth in one go and collects up all the output states.
// The set difference of these new states and the previously seen
// states is then computed as a batch operation, producing the
// states to use as input for the next depth.
//
// The main reason for doing the deduplication in a large batch is to
// keep the memory footprint down. All data processing happens in a
// streaming manner, there are essentially no random accesses at
// all. This allows for keeping the state on disk when its size
// exceeds the available physical memory, without totally destroying
// performance. In addition the streaming access pattern + the
// specific data structure used allows for compressing the data at
// rest with very good compression ratios (10x), with decompression
// happening as the data is streamed for reading.
//
// Despite memory footprint being the main reason for this variant
// of the algorithm, it appears to be faster than the standard
// hash table based variants on problems of non-trivial size, even
// if all the data fits into memory. This is most likely due to
// hash tables being very cache unfriendly, while the access pattern
// used here is very easy for the hardware prefetcher to work with.
//
// Let's call a sorted sequence of states a run.
//
// - For each state generated at previous depth ("todo run"), generate
//   all outputs. Collect them in a vector of new states.
// - Sort + deduplicate the new states, generating a run.
//   - If the number of output states grows too large, generate
//     multiple runs.
// - Iterate through the all the "new state" runs and the run
//   of "seen states" in lockstep.
//   - If this iteration ever sees a state that's present in the
//     new runs but not in the old one, collect that state into the
//     new todo set. Since the runs are sorted, this is effectively
//     a set difference operation.
//   - Add a new run to the collection of old states, consisting of
//     the states kept in the last step.
//   - Add all distinct states to a new "seen states" run.
// - Replace the old todo run and old seen states run with the outputs
//   of the merge.

#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>
#include <vector>

#include "compress.h"
#include "file-backed-array.h"

// A default policy class, with hook implementations that do nothing.
template<class State, class FixedState>
struct BFSPolicy {
    // Called at the start of each new depth of the breadth-first
    // search.
    static void start_iteration(int depth) {
    }

    // Called for every state on the solution that was found.
    static void trace(const FixedState& setup, const State& state, int depth) {
    }
};

// A breadth first search driven by the template parameters.
//
// Template parameters.
//
// State: A node in the state graph. Must implement:
// - do_valid_moves(const FixedState& setup,
//                  std::function<bool(State)> fun) const
//   Calls fun with all states that can be reached from this
//   state.
// - win(): Returns true if the state is in a win condition.
// - print(const FixedState& setup): Prints the state to stdout.
// - Must have a default constructor, which must represent a state
//   that is not reachable from any other state.
//
// FixedState: An opaque scenario description, containing data that's
// needed for interpreting the State objects but that's identical
// between all states. A single FixedState object will be passed to
// the main entry point, and threaded through all computations.
//
// Policy: Hook functions called at various point of the search
// process. See BFSPolicy for the set of hooks that should be
// defined.
//
// PackedState: A byte serialization of State.
// - width_bytes(): Returns the maximum possible width of the
//   serialization, for any possible state.
// - bytes(): Returns a mutable array of width_bytes() bytes.
// - hash(): Returns a hash code.
// - operator< and operator==: PackedStates must have a total order.
// - There must be mutual constructors from PackedState to State
//   and vice versa.
template<class State, class FixedState,
         class Policy = BFSPolicy<State, FixedState>,
         class PackedState = typename State::Packed,
         bool Compress = true>
class BreadthFirstSearch {
public:
    // The serialized states are the main key type for our data structures.
    using Key = PackedState;
    // The data associated with a Key (generally the hash-code of the
    // parent state that generated the state).
    using Value = uint8_t;
    using st_pair = std::pair<Key, Value>;

    // A sequence of serialized states. (Note that each state is
    // likely to serialize to multiple bytes, so a single element of
    // this array represents just a part of the state).
    using Keys = file_backed_mmap_array<uint8_t>;
    // A sequence of values bound to states. It's expected that there
    // is exactly one value per key, and that the values and keys are
    // in the same order.
    using Values = file_backed_mmap_array<uint8_t>;
    // The keys / values collected during a single iteration of the
    // search.
    using NewStates = std::vector<st_pair>;
    // A byte range point to a Keys array, indicating the start/end of
    // a sorted sequence of keys.
    using KeyRun = Keys::Run;

    using KeyStream = StructureDeltaDecompressorStream<Key, Compress>;
    using ValueStream = PointerStream<Value>;
    using KeyCompressor = ByteArrayDeltaCompressor<Key::width_bytes(),
                                                   Compress,
                                                   Keys>;

    // Execute a search from start_state to any win state.
    int search(State start_state, const FixedState& setup) {
        State null_state;

        // BFS state

        // The unique states that were generated at some earlier
        // depth. Each state will be in keys_by_depth just once. The
        // new states generated from depth N in the search will be
        // in the Nth sorted run in the array.
        Keys keys_by_depth;
        // The values associated to the states in keys_by_depth, in
        // the same order.
        Values values_by_depth;
        // The same keys as in all_keys, but in a single sorted run.
        // This array gets recreated on every depth.
        Keys all_keys;
        // The winning state, if one has been found.
        st_pair win_state { null_state, 0 };
        st_pair start_st { start_state, 0 };

        // Initialize the data structures with the start state.
        {
            Keys::WriteRun key_writer { &keys_by_depth };
            Values::WriteRun value_writer { &values_by_depth };

            KeyCompressor compress { &keys_by_depth };
            compress.pack(start_st.first.bytes());
            values_by_depth.push_back(0);
        }

        {
            Keys::WriteRun key_writer { &all_keys };
            KeyCompressor compress { &all_keys };
            compress.pack(start_st.first.bytes());
        }

        for (int iter = 0; ; ++iter) {
            Policy::start_iteration(iter);

            // The new states generated on this iteration, in some
            // number of sorted runs.
            Keys new_keys;
            // The values associated with the new states, in the same
            // order as new_keys.
            Values new_values;

            // The latest run in keys_by_depth will contain all the
            // states we know about but have not yet visited.
            auto last_run = keys_by_depth.run(keys_by_depth.run_count() - 1);
            if (last_run.first == last_run.second) {
                // We haven't won, and have no moves to process.
                return 0;
            }

            bool win = visit_states(setup, last_run, &new_keys, &new_values,
                                    &win_state);

            printf("  new states: %ld\n", new_values.size());
            fflush(stdout);

            // Find all the new states that had never been generated
            // at an earlier depth. These states will be written out
            // to keys as a new run. All other new states will be
            // discarded.
            int uniq = dedup(&keys_by_depth, &values_by_depth, &all_keys,
                             new_keys, new_values);
            printf("  new unique: %d\n", uniq);
            printf("  total size: %ld / %ld\n", all_keys.size(),
                   values_by_depth.size());

            if (win) {
                break;
            }
        }

        return trace_solution_path(setup, keys_by_depth, values_by_depth, win_state);
    }


private:

    // Visits all states in run. Writes the generated states into
    // one or more runs of new_keys and new_values. If a winning
    // state is found, sets it to win_state and returns true.
    bool visit_states(const FixedState& setup, const KeyRun& run,
                      Keys* new_keys, Values* new_values,
                      st_pair* win_state) {
        // A temporary collection of new states / values. Will
        // get flushed into new_keys / new_values either when
        // it grows too large or once we've dealt with the whole
        // todo queue.
        NewStates new_states;
        bool win = false;

        // Visit all the states added on the last depth.
        for (KeyStream todo(run.first, run.second); todo.next(); ) {
            State st(todo.value());
            auto parent_hash = todo.value().hash();

            // For each state collect the possible output states.
            st.do_valid_moves(setup,
                              [&new_states, &parent_hash, &win_state,
                               &win]
                              (State new_state) {
                                  st_pair pair(new_state,
                                               parent_hash & 0xff);
                                  new_states.push_back(pair);
                                  if (new_state.win()) {
                                      *win_state = pair;
                                      win = true;
                                      return true;
                                  }
                                  return false;
                              });
            // If we collect too many new states, do an
            // intermediate deduplication + compression step now.
            if (new_states.size() > 100000000) {
                pack_pairs(&new_states, new_keys, new_values);
            }
        }
        // Dedup + compression any leftovers.
        pack_pairs(&new_states, new_keys, new_values);

        return win;
    }

    // Works backwards from the winning state to the start state,
    // calling Policy::trace on each state. Note that this function
    // takes advantage of the original keys_by_depth having one run per
    // depth; it should not be called with compacted_keys_by_depth.
    int trace_solution_path(const FixedState& setup,
                            const Keys& keys_by_depth,
                            const Values& values_by_depth,
                            const st_pair win_state) {
        st_pair target = win_state;

        int depth = keys_by_depth.run_count();

        for (int i = depth - 1; i > 0; --i) {
            Policy::trace(setup, State(target.first), i);

            auto runinfo = keys_by_depth.run(i - 1);
            // Work through all the states at a given depth.
            KeyStream stream(runinfo.first, runinfo.second);

            bool found_next = false;
            for (int j = 0; stream.next(); ++j) {
                const auto& key = stream.value();
                st_pair current(key, 0);

                // Look for states whose hash matches the value of
                // the current states. (Since we've stored the hash
                // of each state's parent in the value). This allows
                // us to skip the full expensive test for all but
                // one in 256 states.
                if ((key.hash() & 0xff) != (target.second & 0xff)) {
                    continue;
                }

                // If the hashes matched, this is a potential parent
                // of the current state. Try generating the output
                // states for the potential parent, and check if any
                // of them match the current one.
                State st(key);
                if (st.do_valid_moves(setup,
                                      [&target](State new_state) {
                                          Key p(new_state);
                                          if (p == target.first) {
                                              return true;
                                          }
                                          return false;
                                      })) {
                    // Got a match; set the potential parent as the
                    // current state.
                    const auto& value = values_by_depth.run(i - 1).first[j];
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

    // Given a vector of newly generated states+value pairs,
    // deduplicates the states against other states in the same
    // vector. If there are multiple pairs with identical states
    // but different values, keeps an arbitrary pair.
    //
    // Writes the states (in sorted order) to new_keys.
    // Writes the values (in the same order as the states) to new_values.
    void pack_pairs(NewStates* new_states, Keys* new_keys,
                    Values* new_values) {
        std::sort(new_states->begin(), new_states->end(),
                  [] (const st_pair& a, const st_pair& b) {
                      return a.first < b.first;
                  });
        Key prev;

        Keys::WriteRun key_writer { new_keys };
        Values::WriteRun value_writer { new_values };
        KeyCompressor compress { new_keys };
        for (const auto& pair : *new_states) {
            if (pair.first == prev) {
                continue;
            }
            compress.pack(pair.first.bytes());
            new_values->push_back(pair.second);
            prev = pair.first;
        }

        new_states->clear();
    }

    // Finds all states in new_keys that are not present in
    // all_keys. Adds them to keys_by_depth as a new run. Rewrites
    // all_keys to be a single run that's a union of new_keys
    // and the original all_keys.
    //
    // Returns the number of states added to all_keys.
    size_t dedup(Keys* keys_by_depth, Values* values_by_depth,
                 Keys* all_keys,
                 const Keys& new_keys, const Values &new_values) {
        // Set to true iff a state should be discarded due to the
        // presence of an earlier duplicate.
        std::vector<bool> discard(new_values.size());
        Keys new_all_keys;

        using PairStream = StreamPairer<Key, Value, KeyStream, ValueStream>;

        // Iterate through the new keys / values in tandem. If new_*
        // has multiple runs, interleave the runs togehter into a
        // single sorted stream.
        SortedStreamInterleaver<typename PairStream::Pair, PairStream>
            new_stream;
        for (int run = 0; run < new_keys.run_count(); ++run) {
            auto keyinfo = new_keys.run(run);
            auto valinfo = new_values.run(run);
            if (keyinfo.first != keyinfo.second) {
                auto keystream =
                    new KeyStream(keyinfo.first, keyinfo.second);
                auto valstream =
                    new ValueStream(valinfo.first, valinfo.second);
                new_stream.add_stream(new PairStream(keystream,
                                                     valstream));
            }
        }

        {
            Keys::WriteRun key_writer { keys_by_depth };
            Values::WriteRun value_writer { values_by_depth };
            Keys::WriteRun all_keys_writer { &new_all_keys };

            KeyStream merged_stream { all_keys->begin(),
                    all_keys->end() };

            KeyCompressor compress { keys_by_depth };
            KeyCompressor compress_merged { &new_all_keys };

            merged_stream.next();
            new_stream.next();

            while (1) {
                bool have_new = !new_stream.empty();
                bool have_old = !merged_stream.empty();
                if (have_new && have_old) {
                    auto new_st = new_stream.value().first;
                    auto old_st = merged_stream.value();
                    if (old_st < new_st) {
                        compress_merged.pack(old_st.bytes());
                        merged_stream.next();
                    } else if (old_st == new_st) {
                        compress_merged.pack(old_st.bytes());
                        merged_stream.next();
                        new_stream.next();
                    } else {
                        compress.pack(new_st.bytes());
                        compress_merged.pack(new_st.bytes());
                        values_by_depth->push_back(new_stream.value().second);
                        new_stream.next();
                    }
                } else if (have_old) {
                    auto old_st = merged_stream.value();
                    compress_merged.pack(old_st.bytes());
                    merged_stream.next();
                } else if (have_new) {
                    auto new_st = new_stream.value().first;
                    compress.pack(new_st.bytes());
                    compress_merged.pack(new_st.bytes());
                    values_by_depth->push_back(new_stream.value().second);
                    new_stream.next();
                } else {
                    break;
                }
            }
        }

        std::swap(*all_keys, new_all_keys);

        auto last_run = values_by_depth->run(values_by_depth->run_count() - 1);
        size_t count = std::distance(last_run.first, last_run.second);
        return count;
    }
};

#endif

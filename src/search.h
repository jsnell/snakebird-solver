// -*- mode: c++ -*-

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
// - For each state generated at previous depth, generate all
//   output depths. Collect them in a vector of new states.
// - Sort + deduplicate the new states, generating a run.
// - Iterate through the run of new states and all the runs of old
//   states, in lock-step.
// - If this iteration ever sees a state that's present in the
//   new run but not in the old ones, collect that state. Since
//   the runs are sorted, this is effectively a set difference
//   operation.
// - Add a new run to the collection of old states, consisting of
//   the states kept in the last step.
//
// Since iterating through N sorted sequences with a total of M items
// is an O(M log N) operation, we occasionally merge the old runs
// together into one larger run. This is effectively a poor man's
// log-structured merge tree.
//
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
        // depth.  Each state will be in seen_keys just once. Each
        // depth in the search will be represented as exactly one run
        // in these arrays, with the run being sorted.
        Keys seen_keys;
        // The values associated with the states in seen_keys, in the
        // same order.
        Values seen_values;
        // All states generated at the current depth.
        NewStates new_states;
        // The keys / values generated at the current depth, deduplicated
        // and sorted by key.
        Keys new_keys;
        Values new_values;
        // True if a win condition has been found.
        bool win = false;
        // The winning state. Only valid if win==true.
        st_pair win_state { null_state, 0 };

        // An optimization. Keys will occasionally be copied from
        // seen_keys to compacted_seen_keys, with the states of
        // multiple runs from seen_keys being written out as a single
        // sorted run in compacted_seen_keys. This makes merging the
        // KeyStreams much faster, and also improves the compression
        // ratio.
        Keys compacted_seen_keys;
        // The number of runs / states in seen_keys that have not been
        // written out to compacted_seen_keys yet.
        size_t uncompacted_runs = 0;
        size_t uncompacted_states = 0;

        // Initialize our state queue with the starting state.
        new_states.emplace_back(start_state, 0);

        seen_keys.freeze();
        compacted_seen_keys.freeze();

        size_t new_state_count = 0;
        for (int iter = 0; ; ++iter) {
            Policy::start_iteration(iter);

            // Deduplicate the new states.
            {
                new_state_count += new_states.size();
                pack_pairs(&new_states, &new_keys, &new_values);
                new_keys.freeze();
                new_values.freeze();
            }

            printf("  new states: %ld\n", new_state_count);
            new_state_count = 0;
            fflush(stdout);

            // Find all the new states that had never been generated
            // at an earlier depth. These states will be written out
            // to keys as a new run. All other new states will be
            // discarded.
            {
                std::vector<KeyRun> runs = compacted_seen_keys.runs();
                std::vector<KeyRun> uncompacted = seen_keys.runs();
                // The same data might be present in both seen_keys
                // and compacted_seen_keys. Use the data from the latter
                // when possible, then fill in the rest from the former.
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

            // The latest run will contain all the new states.
            auto todo = seen_keys.run(seen_keys.run_count() - 1);

            if (todo.first == todo.second) {
                // We haven't won, and have no moves to process.
                return 0;
            }

            // If we've got too many separate runs in seen_keys, merge
            // them together into a new run in compacted_seen_keys. But
            // don't do this if the runs are trivially small.
            if (uncompacted_runs >= 8 &&
                uncompacted_states >= 1000000) {
                std::vector<KeyRun> to_compact = seen_keys.runs();
                to_compact.erase(to_compact.begin(),
                                 to_compact.end() - uncompacted_runs);
                auto prevsize = compacted_seen_keys.size();
                compact_runs(&compacted_seen_keys, to_compact);
                printf("Compacted %ld runs with %ld states, "
                       "orig bytes: %ld new bytes: %ld\n",
                       uncompacted_runs,
                       uncompacted_states,
                       (to_compact.back().second - to_compact.front().first),
                       compacted_seen_keys.size() - prevsize);
                uncompacted_runs = 0;
                uncompacted_states = 0;
            }

            KeyStream stream(todo.first, todo.second);

            // Process the actual new states from the last depth.
            while (stream.next()) {
                State st(stream.value());
                auto parent_hash = stream.value().hash();

                // For each state collect the possible output states.
                st.do_valid_moves(setup,
                                  [&new_states, &parent_hash, &win_state,
                                   &win]
                                  (State new_state) {
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
                // If we collect too many new states, do an
                // intermediate deduplication + compression step now.
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
    // Works backwards from the winning state to the start state,
    // calling Policy::trace on each state. Note that this function
    // takes advantage of the original seen_keys having one run per
    // depth; it should not be called with compacted_seen_keys.
    int trace_solution_path(const FixedState& setup,
                            const Keys& seen_keys,
                            const Values& seen_values,
                            const st_pair win_state) {
        st_pair target = win_state;

        int depth = seen_keys.run_count();

        for (int i = depth - 1; i > 0; --i) {
            Policy::trace(setup, State(target.first), i);

            auto runinfo = seen_keys.run(i - 1);
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

        new_keys->start_run();
        new_values->start_run();
        {
            KeyCompressor compress { new_keys };
            for (const auto& pair : *new_states) {
                if (pair.first == prev) {
                    continue;
                }

                compress.pack(pair.first.bytes());
                new_values->push_back(pair.second);
                prev = pair.first;
            }
        }
        new_keys->end_run();
        new_values->end_run();

        new_states->clear();
    }


    // Given a vector of runs, generates a stream for the run
    // and adds the stream to the given stream interleaver.
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

    // Finds all states in new_keys that are not present in
    // seen_keys_runs. Adds them to seen_keys as a new run.
    size_t dedup(Keys* seen_keys, Values* seen_values,
                 const std::vector<KeyRun>& seen_keys_runs,
                 const Keys& new_keys, const Values &new_values) {
        // Set to true iff a state should be discarded due to the
        // presence of an earlier duplicate.
        std::vector<bool> discard(new_values.size());

        using PairStream = StreamPairer<Key, Value, KeyStream, ValueStream>;

        if (new_keys.size()) {
            // Process all the existing keys in order, rather than
            // a run at a time.
            SortedStreamInterleaver<Key, KeyStream> seen_keys_stream;
            add_stream_runs<KeyStream>(&seen_keys_stream,
                                       seen_keys_runs);

            // new_keys can have multiple runs if new_states got
            // flushed early on. Process all the states in order too.
            // (This is necessary, not just an optimization, since the
            // same key can be present in multiple runs of new_keys.
            // SortedStreamInterleaver will remove those duplicates for
            // us.)
            SortedStreamInterleaver<Key, KeyStream> new_keys_stream;
            add_stream_runs<KeyStream>(&new_keys_stream, new_keys.runs());

            new_keys_stream.next();

            size_t i = 0;
            while (seen_keys_stream.next()) {
                // If the next new key is smaller than the current
                // seen_key, keep the new key.
                while (new_keys_stream.value() < seen_keys_stream.value()) {
                    if (!new_keys_stream.next()) {
                        // Out of keys.
                        goto done;
                    }
                    ++i;
                }
                // If the next new key matches an old key, throw it away.
                if (new_keys_stream.value() == seen_keys_stream.value()) {
                    discard[i] = true;
                }
            }
        done:
            ;
        }

        size_t count = 0;
        seen_keys->thaw();
        seen_keys->start_run();
        seen_values->start_run();

        // Iterate through the new keys again, in the same order as above.
        // But this time pair them up with the matching value.
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

        // Write out any keys + values that weren't marked for disposal.
        {
            KeyCompressor compress { seen_keys };
            for (int i = 0; new_state_stream.next(); ++i) {
                if (!discard[i]) {
                    ++count;
                    compress.pack(new_state_stream.value().first.bytes());
                    seen_values->push_back(new_state_stream.value().second);
                }
            }
        }

        seen_keys->freeze();
        seen_keys->end_run();
        seen_values->end_run();

        return count;
    }

    // Given a list of runs of states, merges the states together
    // into one sorted run in output.
    size_t compact_runs(Keys* output,
                        const std::vector<KeyRun>& runs) {
        SortedStreamInterleaver<Key, KeyStream> stream;
        add_stream_runs<KeyStream>(&stream, runs);
        output->thaw();
        output->start_run();

        size_t count = 0;

        {
            KeyCompressor compress { output };
            for (; stream.next(); ++count) {
                compress.pack(stream.value().bytes());
            }
        }

        output->end_run();
        output->freeze();
        return count;
    }
};

#endif

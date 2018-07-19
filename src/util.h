// -*- mode: c++ -*-

#ifndef UTIL_H
#define UTIL_H

#include <cassert>
#include <chrono>
#include <memory>
#include <queue>

// Compute the length of an integer (i.e. position of first 1 bit)
// at compile-time.
template<uint64_t I>
struct integer_length {
    enum { value = 1 + integer_length<I/2>::value, };
};

template<>
struct integer_length<1> {
    enum { value = 1 };
};

// Returns a bitmask with the lowest n bits set to 1, all other bits
// set to 0.
constexpr uint64_t mask_n_bits(uint64_t n) {
    return (UINT64_C(1) << n) - 1;
}

// Streams:
//
// Streams are a lazily computed sequence of records of a given
// type. Streams have the following interface:
//
// - empty() - Returns true if the sequence has been exhausted;
//   no operations other than calls to empty() or deallocation
//   should be done on an empty stream.
// - next() - Fetches the next record. Returns true if a record
//   was fetched, false otherwise. In the latter case, the stream
//   will be considered empty (with all the previously stated
//   restrictions).
// - value() - Returns a reference to the latest decoded record (may not be
//   called if no records have been read yet). Valid only until next
//   call to next().

// A Stream that treats the memory range "begin" to "end" as
// an array of T.
template<class T>
struct PointerStream {
    PointerStream(const T* begin, const T* end)
        : begin_(begin - 1), end_(end) {
    }

    bool empty() const { return begin_ == end_; }
    bool next() { ++begin_; return !empty(); }
    const T& value() const { return *begin_; }

private:
    const T* begin_;
    const T* end_;
};

// A Stream that merges together multiple streams. value()
// and next() will only operate on the stream whose head value
// is the smallest. The unstated assumption is that the
// component streams are sorted.
//
// If DeleteDuplicates is true, ensures there are no adjacent
// duplicates in the output stream. That is:
//
//   a = value(); next(); b = value();
//   assert(a != b);
//
// This is internally implemented using a heap-based priority
// queue.
template<class T, class Stream, bool DeleteDuplicates=true>
struct SortedStreamInterleaver {
    SortedStreamInterleaver() {
    }

    ~SortedStreamInterleaver() {
        while (!streams_.empty()) {
            delete streams_.top();
            streams_.pop();
        }
    }

    // Registers "stream" as one of the component streams that
    // will be merged together. Takes ownership of the stream.
    void add_stream(Stream* stream) {
        if (stream->next()) {
            streams_.push(stream);
            empty_ = false;
        } else {
            delete stream;
        }
    }

    bool empty() {
        return empty_;
    }

    bool next() {
        if (streams_.empty()) {
            empty_ = true;
            return false;
        }

        // God, the priority_queue interface is hard to use.
        auto top_stream = streams_.top();
        T value = top_stream->value();
        streams_.pop();
        if (top_stream->next()) {
            streams_.push(top_stream);
        } else {
            delete top_stream;
        }

        if (DeleteDuplicates) {
            if (value == top_) {
                return next();
            }
        }

        top_ = value;
        return true;
    }

    const T& value() const {
        return top_;
    }

private:
    T top_;
    bool empty_ = false;

    struct Cmp {
        bool operator()(const Stream* a, const Stream* b) {
            // Note: invert the comparison since priority_queue
            // is by default a max-heap rather than a min-heap.
            return *b < *a;
        }
    };

    std::priority_queue<Stream*, std::vector<Stream*>, Cmp> streams_;
};

// A stream that takes two input streams of type K and V, and produces
// output records of type Pair<K, V>. If the input streams are of
// a different length, is exhausted when the first of the input
// streams is exhausted.
template<class K, class V, class KeyStream, class ValueStream>
struct StreamPairer {
    // Like std::pair, except comparisons only affect "first".
    class Pair {
    public:
        Pair() {
        }

        Pair(const K& key, V value)
            : first(key), second(value) {
        }

        bool operator<(const Pair& other) const {
            return first < other.first;
        }
        bool operator==(const Pair& other) const {
            return first == other.first;
        }

        K first;
        V second;
    };

    // Does not take ownership of either of the input streams.
    StreamPairer(KeyStream* keys, ValueStream* values)
        : keys_(keys),
          values_(values) {
    }

    bool next() {
        bool kn = keys_->next();
        bool vn = values_->next();
        if (!kn || !vn) {
            empty_ = true;
            return false;
        }

        pair_.first = keys_->value();
        pair_.second = values_->value();
        return true;
    }

    bool empty() const {
        return empty_;
    }

    const Pair& value() const {
        return pair_;
    }

    bool operator<(const StreamPairer& other) const {
        return *keys_ < *other.keys_;
    }

private:
    bool empty_ = false;
    Pair pair_;
    std::unique_ptr<KeyStream> keys_;
    std::unique_ptr<ValueStream> values_;
};

#endif

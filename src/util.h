// -*- mode: c++ -*-

#ifndef UTIL_H
#define UTIL_H

#include <cassert>
#include <chrono>
#include <memory>
#include <queue>

template<uint64_t I>
struct integer_length {
    enum { value = 1 + integer_length<I/2>::value, };
};

template<>
struct integer_length<1> {
    enum { value = 1 };
};

constexpr uint64_t mask_n_bits(uint64_t n) {
    return (UINT64_C(1) << n) - 1;
}

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

    void add_stream(Stream* stream) {
        assert(stream->next());
        streams_.push(stream);
    }

    bool next() {
        if (streams_.empty()) {
            return false;
        }

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
            return *b < *a;
        }
    };

    std::priority_queue<Stream*, std::vector<Stream*>, Cmp> streams_;
};

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

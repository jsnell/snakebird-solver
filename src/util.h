// -*- mode: c++ -*-

#ifndef UTIL_H
#define UTIL_H

#include <cassert>
#include <chrono>
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
    return (1 << n) - 1;
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

#endif

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

template<class T>
class MultiMerge {
public:
    struct Source {
        Source(T* begin, T* end) : begin(begin), end(end) {
        }

        bool operator<(const Source& other) const {
            // Reverse comparison since pqueue defaults to having
            // the largest element at the top.
            return *other.begin < *begin;
        }

        const T& value() const {
            return *begin;
        }

        T* begin;
        T* end;
    };

    MultiMerge(std::function<void(const T&)> output) : output_(output) {
    }

    void add_input_source(T* begin, T* end) {
        if (begin != end) {
            state_.push(Source(begin, end));
        }
    }

    void merge() {
        while (!state_.empty()) {
            auto top = state_.top();
            output_(top.value());
            state_.pop();
            add_input_source(top.begin + 1, top.end);
        }
    }

private:
    std::priority_queue<Source> state_;
    std::function<void(const T&)> output_;
};

#endif

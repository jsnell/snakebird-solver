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

#endif

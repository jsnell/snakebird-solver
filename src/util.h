// -*- mode: c++ -*-

#ifndef UTIL_H
#define UTIL_H

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

#endif

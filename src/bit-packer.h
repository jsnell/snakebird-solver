// -*- mode: c++ -*-

#ifndef BIT_PACKER_H
#define BIT_PACKER_H

#include "util.h"

template<size_t Bits>
struct Packer {
    static const int Bytes = (Bits + 7) / 8;

    Packer() {
    }

    struct Context {
        uint64_t acc_ = 0;
        size_t acc_bits_ = 0;
        size_t at_ = 0;
    };

    template<typename T>
    void deposit(T data, size_t width, Context* context) {
        assert(width <= 64);
        if (context->acc_bits_ + width > 64) {
            flush(context);
        }
        context->acc_ |= (uint64_t) data << context->acc_bits_;
        context->acc_bits_ += width;
    }

    void flush(Context* context) {
        uint64_t data = context->acc_;
        uint64_t width = context->acc_bits_;
        size_t at = context->at_;

        while (width) {
            size_t offset = (at % 8);
            int bits_to_deposit = std::min(width, 8 - offset);
            int deposit_at = at / 8;
            bytes_[deposit_at] |= data << offset;
            data >>= bits_to_deposit;
            at += bits_to_deposit;
            width -= bits_to_deposit;
        }

        context->acc_ = 0;
        context->acc_bits_ = 0;
        context->at_ = at;
    }

    template<typename T>
    void extract(T& data, size_t width, Context* context) const {
        if (context->acc_bits_ < width) {
            refill(context);
        }
        data = context->acc_ & mask_n_bits(width);
        context->acc_ >>= width;
        context->acc_bits_ -= width;
    }

    void refill(Context* context) const {
        while (context->acc_bits_ <= 56 &&
               context->at_ < Bytes) {
            context->acc_ |=
                (uint64_t) bytes_[context->at_++] << context->acc_bits_;
            context->acc_bits_ += 8;
        }
    }

    uint8_t bytes_[Bytes] = { 0 };
};

#endif // BIT_PACKER_H

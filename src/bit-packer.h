// -*- mode: c++ -*-

#ifndef BIT_PACKER_H
#define BIT_PACKER_H

#include "util.h"

// A class for serializing/deserializing a stream of variable
// width fields (max _Bits_ bits in total) into a byte stream
// of fixed length.
//
// Serialization example:
//
//   int a = 42, b = 31;
//   Packer<63> data;
//   Packer::Context pack;
//   data.deposit(a, 6, &pack);
//   data.deposit(b, 7, &pack);
//   memcpy(out, data.bytes_, data::Bytes);
//
// Deserialization example:
//
//   int a = 42, b = 31;
//   Packer<63> data;
//   memcpy(data.bytes_, in, data::Bytes);
//   Packer::Context unpack;
//   data.extract(a, 6, &unpack);
//   data.extract(b, 7, &unpack);
template<size_t Bits>
struct Packer {
    // Size of the serialized data.
    static const int Bytes = (Bits + 7) / 8;

    // The serialized data. Should be initialized to contain the
    // appropriate bytes when deserializing. When serializing,
    // will contain the serialized byte stream after a call to
    // flush().
    uint8_t bytes_[Bytes] = { 0 };

    // Serialization / deserialization context. Should be considered
    // opaque internal data.
    struct Context {
        // Data that's been read from the backing buffer but not
        // yet returned by extract(), or data that's been written
        // with deposit() but not yet flushed.
        uint64_t acc_ = 0;
        // How many bits of acc_ are valid.
        size_t acc_bits_ = 0;
        // The bytes that have been fully read / written from the
        // backing buffer.
        size_t at_ = 0;
    };

    // Store the lowest _width_ bits from _data_ into the appropriate
    // bits of the buffer. Might cause a flush of the buffer into
    // the backing array.
    template<typename T>
    void deposit(T data, size_t width, Context* context) {
        assert(width <= 64);
        if (context->acc_bits_ + width > 64) {
            flush(context);
        }
        context->acc_ |= (uint64_t) data << context->acc_bits_;
        context->acc_bits_ += width;
    }

    // Ensures all deposited data is written from the buffer to the
    // backing array.
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

    // Reads the next _width_ bits from the backing array, and
    // stores them in _data_.
    template<typename T>
    void extract(T& data, size_t width, Context* context) const {
        if (context->acc_bits_ < width) {
            refill(context);
        }
        data = context->acc_ & mask_n_bits(width);
        context->acc_ >>= width;
        context->acc_bits_ -= width;
    }

private:
    // Fill acc_ from bytes_. (Only reads full bytes; this makes 56
    // bits the maximum field size that's guaranteed to work.).
    void refill(Context* context) const {
        while (context->acc_bits_ <= 56 &&
               context->at_ < Bytes) {
            context->acc_ |=
                (uint64_t) bytes_[context->at_++] << context->acc_bits_;
            context->acc_bits_ += 8;
        }
    }
};

#endif // BIT_PACKER_H

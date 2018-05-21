// -*- mode: c++ -*-

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstring>
#include <string>
#include <vector>

#include <third-party/snappy/snappy.h>
#include <third-party/snappy/snappy-sinksource.h>

template<int Width>
class VarInt {
public:
    static uint64_t decode(const uint8_t*& it) {
        uint8_t byte = *it++;
        if (Width <= 8) {
            return byte;
        }
        if (Width <= 15) {
            if (!(byte & (1 << 7))) {
                return byte;
            }
            return byte | (*it++ << 7);
        }
        assert(false);
    }

    static void encode(uint64_t value, std::function<void(uint8_t)> emit) {
        if (Width <= 8) {
            emit(value);
            return;
        }
        if (Width <= 15) {
            if (value <= mask_n_bits(7)) {
                emit(value);
                return;
            }
            emit((value & mask_n_bits(7)) | (1 << 7));
            emit(value >> 7);
            return;
        }
        assert(false);
    }
};

template<int Length, bool Compress=false>
class SortedStructDecompressor {
public:
    SortedStructDecompressor(const uint8_t* begin, const uint8_t* end) {
        if (Compress) {
            snappy::Uncompress((char*) begin, std::distance(begin, end),
                               &uncompressed_);
            it_ = (const uint8_t*) uncompressed_.data();
            end_ = it_ + uncompressed_.size();
        } else {
            it_ = begin;
            end_ = end;
        }
    }

    bool unpack(uint8_t value[Length]) {
        if (it_ == end_) {
            return false;
        }
        unpack_internal(value);

        return true;
    }

private:
    void unpack_internal(uint8_t output[Length]) {
        uint64_t n = VarInt<Length>::decode(it_);
        while (n) {
            uint64_t mask = n & -n;
            int bit = __builtin_ctzl(mask);
            output[bit] = *it_++;
            n ^= mask;
        }
    }

    std::string uncompressed_;
    const uint8_t* it_;
    const uint8_t* end_;
};

template<int Length, bool Compress, class Output>
class SortedStructCompressor {
public:
    SortedStructCompressor(Output* output) : output_(output) {
    }

    void pack(const uint8_t value[Length]) {
        uint64_t n = 0;
        for (int j = 0; j < Length; ++j) {
            if (prev_[j] != value[j]) {
                n |= 1 << j;
            }
        }

        VarInt<Length>::encode(n,
                               [this] (uint8_t value) {
                                   record(value);
                               });
        for (int j = 0; j < Length; ++j) {
            if (n & (1 << j)) {
                record(value[j]);
                prev_[j] = value[j];
            }
        }
    }

    void write() {
        if (Compress) {
            write_compressed();
        }
    }

private:
    void record(uint8_t byte) {
        if (Compress) {
            delta_transformed_.push_back(byte);
        } else {
            output_->push_back(byte);
        }
    }

    void write_compressed() {
        snappy::ByteArraySource source((char*) &delta_transformed_[0],
                                       delta_transformed_.size());
        struct SnappySink : snappy::Sink {
            SnappySink(Output* out) : out(out) {
            };

            virtual void Append(const char* bytes, size_t n) override {
                for (int i = 0; i < n; ++i) {
                    out->push_back(bytes[i]);
                }
            }

            Output* out;
        };
        SnappySink sink(output_);
        snappy::Compress(&source, &sink);
    }

    uint8_t prev_[Length] = { 0 };
    std::vector<uint8_t> delta_transformed_;
    Output* output_;
};

#endif // COMPRESS_H

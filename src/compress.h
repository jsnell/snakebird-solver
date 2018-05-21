// -*- mode: c++ -*-

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstring>
#include <string>
#include <vector>

#include <third-party/snappy/snappy.h>
#include <third-party/snappy/snappy-sinksource.h>

template<int Length>
class SortedStructDecompressor {
public:
    SortedStructDecompressor(uint8_t* begin, uint8_t* end) {
        snappy::Uncompress((char*) begin, std::distance(begin, end),
                           &uncompressed_);
        it_ = (const uint8_t*) uncompressed_.data();
        end_ = it_ + uncompressed_.size();
    }

    bool unpack(uint8_t value[Length]) {
        if (it_ == end_) {
            return false;
        }
        memcpy(value, prev_, Length);
        unpack_internal(prev_, value);
        memcpy(prev_, value, Length);

        return true;
    }

private:
    void unpack_internal(uint8_t prev[Length],
                         uint8_t output[Length]) {
        uint64_t n = 0;
        for (int i = 0; i < Length; i += 8) {
            uint64_t byte = *it_++;
            n |= byte << i;
        }
        for (int i = 0; i < Length; ++i) {
            if (n & (1 << i)) {
                output[i] = *it_++ ^ prev[i];
            }
        }
    }

    std::string uncompressed_;
    uint8_t prev_[Length] = { 0 };
    const uint8_t* it_;
    const uint8_t* end_;
};

template<int Length, class Output>
class SortedStructCompressor {
public:
    SortedStructCompressor() {
    }

    void pack(const uint8_t value[Length]) {
        uint8_t out[Length];
        uint64_t n = 0;
        for (int j = 0; j < Length; ++j) {
            uint8_t diff = prev_[j] ^ value[j];
            out[j] = diff;
            if (diff) {
                n |= 1 << j;
            }
        }

        for (int i = 0; i < Length; i += 8) {
            delta_transformed_.push_back(n >> i);
        }
        for (int j = 0; j < Length; ++j) {
            if (n & (1 << j)) {
                delta_transformed_.push_back(out[j]);
            }
        }

        memcpy(prev_, value, Length);
    }

    void write(Output* output) {
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
        SnappySink sink(output);
        snappy::Compress(&source, &sink);
    }

private:
    uint8_t prev_[Length] = { 0 };
    std::vector<uint8_t> delta_transformed_;
};

#endif // COMPRESS_H

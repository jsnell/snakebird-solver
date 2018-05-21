// -*- mode: c++ -*-

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstring>
#include <string>
#include <vector>

#include <third-party/snappy/snappy.h>
#include <third-party/snappy/snappy-sinksource.h>

template<int Length, bool Compress=true>
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
        unpack_internal(prev_, value);

        return true;
    }

private:
    void unpack_internal(uint8_t prev[Length],
                         uint8_t output[Length]) {
        uint64_t n = 0;
        for (int i = 0; i < Length; i += 7) {
            uint64_t byte = *it_++;
            uint64_t hibit = byte & (1 << 7);
            n |= (byte & mask_n_bits(7)) << i;
            if (!hibit) {
                break;
            }
        }
        for (int i = 0; i < Length; ++i) {
            if (n & (1 << i)) {
                prev[i] ^= *it_++;
            }
            output[i] = prev[i];
        }
    }

    std::string uncompressed_;
    uint8_t prev_[Length] = { 0 };
    const uint8_t* it_;
    const uint8_t* end_;
};

template<int Length, bool Compress, class Output>
class SortedStructCompressor {
public:
    SortedStructCompressor(Output* output) : output_(output) {
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

        for (int i = 0; i < Length; i += 7) {
            uint64_t hibit = 0;
            if (n >> (i + 7)) {
                hibit = 1 << 7;
            }
            record(((n >> i) & mask_n_bits(7)) | hibit);
            if (!hibit) {
                break;
            }
        }
        for (int j = 0; j < Length; ++j) {
            if (n & (1 << j)) {
                record(out[j]);
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

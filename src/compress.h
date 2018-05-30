// -*- mode: c++ -*-

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstring>
#include <string>
#include <vector>

#include <zstd.h>

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
class ByteArrayDeltaDecompressor {
public:
    ByteArrayDeltaDecompressor(const uint8_t* begin, const uint8_t* end)
        : raw_begin(begin),
          raw_end(end) {
        if (Compress) {
            refill();
        } else {
            it_ = begin;
            end_ = end;
        }
    }

    bool unpack(uint8_t value[Length]) {
        if (it_ == end_ && !refill()) {
            return false;
        }
        unpack_internal(value);

        return true;
    }

private:
    ByteArrayDeltaDecompressor(
        const ByteArrayDeltaDecompressor& other) = delete;
    ByteArrayDeltaDecompressor& operator=(
        const ByteArrayDeltaDecompressor& other) = delete;

    bool refill() {
        if (!Compress ||
            raw_begin == raw_end) {
            return false;
        }

        uint16_t len = *raw_begin++;
        len |= (*raw_begin++) << 8;
        assert(raw_begin + len <= raw_end);
        size_t zsize = ZSTD_getFrameContentSize(raw_begin,
                                                std::distance(raw_begin,
                                                              raw_end));
        if (zbuffer_.size() < zsize) {
            zbuffer_.resize(zsize);
        }
        ZSTD_decompress(&zbuffer_[0],
                        zsize,
                        raw_begin,
                        std::distance(raw_begin, raw_end));
        it_ = &zbuffer_[0];
        end_ = it_ + zsize;
        raw_begin += len;
        return true;
    }

    void unpack_internal(uint8_t output[Length]) {
        uint64_t n = VarInt<Length>::decode(it_);
        while (n) {
            uint64_t mask = n & -n;
            int bit = __builtin_ctzl(mask);
            output[bit] = *it_++;
            n ^= mask;
        }
    }

    std::vector<uint8_t> zbuffer_;
    const uint8_t* it_;
    const uint8_t* end_;
    const uint8_t* raw_begin;
    const uint8_t* raw_end;
};

template<int Length, bool Compress, class Output>
class ByteArrayDeltaCompressor {
public:
    ByteArrayDeltaCompressor(Output* output) : output_(output) {
    }

    ~ByteArrayDeltaCompressor() {
        flush();
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

        if (delta_transformed_.size() > 0x7000) {
            flush();
        }
    }

    void flush() {
        if (Compress) {
            compress_and_flush();
        } else {
            output_->insert_back(delta_transformed_.begin(),
                                 delta_transformed_.end());
            delta_transformed_.clear();
        }
    }

private:
    ByteArrayDeltaCompressor(const ByteArrayDeltaCompressor& other) = delete;
    ByteArrayDeltaCompressor& operator=(
        const ByteArrayDeltaCompressor& other) = delete;

    void record(uint8_t byte) {
        delta_transformed_.push_back(byte);
    }

    void compress_and_flush() {
        char buffer[65536];
        size_t len = ZSTD_compress(buffer, sizeof(buffer),
                                   &delta_transformed_[0],
                                   delta_transformed_.size(),
                                   0);
        output_->push_back(len & 0xff);
        output_->push_back((len >> 8) & 0xff);
        output_->insert_back(&buffer[0], &buffer[len]);
        delta_transformed_.clear();
    }

    uint8_t prev_[Length] = { 0 };
    std::vector<uint8_t> delta_transformed_;
    Output* output_;
};

template<class T, bool Decompress = false>
class StructureDeltaDecompressorStream {
public:
    StructureDeltaDecompressorStream(const uint8_t* begin,
                                     const uint8_t* end,
                                     int id = 0)
        : id_(id), stream_(begin, end) {
    }

    const T& value() const {
        return value_;
    }

    bool empty() {
        return empty_;
    }

    bool next() {
        if (!stream_.unpack(value_.p_.bytes_)) {
            empty_ = true;
        }
        return !empty_;
    }

    bool operator<(const StructureDeltaDecompressorStream& other) const {
        return value_ < other.value_;
    }

private:
    StructureDeltaDecompressorStream(const StructureDeltaDecompressorStream& other) = delete;
    StructureDeltaDecompressorStream& operator=(
        const StructureDeltaDecompressorStream& other) = delete;

    int id_ = 0;
    T value_;
    bool empty_ = false;
    ByteArrayDeltaDecompressor<sizeof(T::p_.bytes_), Decompress> stream_;
};


#endif // COMPRESS_H

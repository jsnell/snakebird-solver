// -*- mode: c++ -*-

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstring>
#include <string>
#include <vector>

#include <zstd.h>


// An encoder / decoder for variable-width integers of at most _Width_
// bits. Uses the classic stop-bit approach, where the 7 low bits have
// payload, the top bit being zero means this is the last byte for
// this value. The last possible byte for a given width will also use
// the top bit for payload. (E.g. a VarInt width of 8 will always
// require exactly one byte).
//
// Only unsigned integers are supported.
template<int Width>
class VarInt {
public:
    static const uint64_t kTopBit = 1 << 7;

    // Reads an integer from the octet pointer _it_. Advances the
    // pointer, returns the integer.
    static uint64_t decode(const uint8_t*& it) {
        uint8_t byte = *it++;
        if (Width <= 8) {
            return byte;
        }
        if (Width <= 15) {
            if (!(byte & kTopBit)) {
                return byte;
            }
            return (byte & 0x7f) | (*it++ << 7);
        }
        if (Width <= 22) {
            if (!(byte & kTopBit)) {
                return byte;
            }
            uint64_t val = byte & ~kTopBit;
            byte = *it++;
            if (!(byte & kTopBit)) {
                return val | byte << 7;
            } else {
                val |= (byte & ~kTopBit) << 7;
            }
            byte = *it++;
            val |= byte << 14;
            return val;
        }
        assert(false);
    }

    // Encodes _value_. Calls _emit_ once for each output byte.
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
            emit((value & mask_n_bits(7)) | kTopBit);
            emit(value >> 7);
            return;
        }
        if (Width <= 22) {
            if (value <= mask_n_bits(7)) {
                emit(value);
                return;
            }
            if (value <= mask_n_bits(14)) {
                emit((value & mask_n_bits(7)) | kTopBit);
                emit((value >> 7) & mask_n_bits(7));
                return;
            }
            emit((value & mask_n_bits(7)) | kTopBit);
            emit(((value >> 7) & mask_n_bits(7)) | kTopBit);
            emit(value >> 14);
            return;
        }
        assert(false);
    }
};

// The following classes deal with compression and decompression of
// streams of (preferably sorted) fixed-size records.
//
// There are two layers of compression going on. The inner layer
// is a radix delta transformation, where a record is encoded as:
//
// <VarInt of _Length_ bits, with N bits set> <N octets>
//
// If bit X is not set in the leading VarInt, the Xth octet in the
// record is the same as in the previous record in the stream. The
// VarInt will have all bits set for the first record.
//
// The optional outer layer is normal zstd compression, with blocks
// that correspond to roughly 32k of plaintext. Each block is
// preceded by the compressed length of the block encoded as a VarInt.


// Decompresses records of _Length_ bytes from an octet buffer
// that's encoded in the format described above. If _Compress_
// is false, only applies the radix delta transform.
template<int Length, bool Compress=false>
class ByteArrayDeltaDecompressor {
public:
    ByteArrayDeltaDecompressor(const uint8_t* begin, const uint8_t* end)
        : raw_it_(begin),
          raw_end_(end) {
        if (Compress) {
            refill();
        } else {
            it_ = begin;
            end_ = end;
        }
    }

    // Reads a record from the buffer, and stores it in _value_.
    // Unless this is the first call to unpack(), _value_ should
    // contain bytes that are equal to the previous record.
    //
    // Returns false if all the records have been read already.
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

    // If the outer compression layer is in use, decompresses a block
    // and points the radix transformer at the uncompressed data.
    // We expect that records will never end up straddling two
    // blocks.
    bool refill() {
        if (!Compress ||
            raw_it_ == raw_end_) {
            return false;
        }

        uint64_t len = VarInt<22>::decode(raw_it_);
        assert(raw_it_ + len <= raw_end_);
        size_t zsize = ZSTD_getFrameContentSize(raw_it_,
                                                std::distance(raw_it_,
                                                              raw_end_));
        if (zbuffer_.size() < zsize) {
            zbuffer_.resize(zsize);
        }
        ZSTD_decompress(&zbuffer_[0],
                        zsize,
                        raw_it_,
                        std::distance(raw_it_, raw_end_));
        it_ = &zbuffer_[0];
        end_ = it_ + zsize;
        raw_it_ += len;
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

    // The block of data from it_ to end_ contains the delta
    // transformed records that unpack() will return.
    const uint8_t* it_;
    const uint8_t* end_;
    // The block of data from raw_it_ to raw_end_ contains data that
    // needs to be decompressed with zstd_.
    const uint8_t* raw_it_;
    const uint8_t* raw_end_;
    // A buffer for the uncompressed data.
    std::vector<uint8_t> zbuffer_;
};


// Compresses records of _Length_ bytes to _output_, using the format
// described above. If _Compress_ is false, only applies the radix
// delta transform.
template<int Length, bool Compress, class Output>
class ByteArrayDeltaCompressor {
public:
    ByteArrayDeltaCompressor(Output* output) : output_(output) {
    }

    ~ByteArrayDeltaCompressor() {
        flush();
    }

    void pack(const uint8_t value[Length]) {
        // Compute a bitmask with bit N set if byte N is different
        // between "value" and the "value" given on the previous
        // call.
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
        // Emit only the changed bytes.
        for (int j = 0; j < Length; ++j) {
            if (n & (1 << j)) {
                // It's tempting to compute some kind of numeric
                // delta here, either with xor or plus/minus. Seems
                // like that should in theory be easier to compress
                // with the later passes. But it didn't work for me
                // in practice.
                record(value[j]);
                prev_[j] = value[j];
            }
        }

        if (delta_transformed_.size() > (1 << 20)) {
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

    // Buffer a byte for possible zstd compression.
    void record(uint8_t byte) {
        delta_transformed_.push_back(byte);
    }

    // Compress the internal accumulator buffer with zstd and write to
    // the result to the output. The compressed data will be prefixed
    // with a Varint representation of the length of the compressed
    // block.
    void compress_and_flush() {
        char buffer[1 << 22];
        size_t len = ZSTD_compress(buffer, sizeof(buffer),
                                   &delta_transformed_[0],
                                   delta_transformed_.size(),
                                   0);
        VarInt<22>::encode(len, [this] (uint8_t byte) {
                output_->push_back(byte);
            });
        output_->insert_back(&buffer[0], &buffer[len]);
        delta_transformed_.clear();
    }

    uint8_t prev_[Length] = { 0 };
    std::vector<uint8_t> delta_transformed_;
    Output* output_;
};

// Given a byte range that's compressed/encoded as above, converts it
// the range to a lazy stream of records of type T.
template<class T, bool Decompress = false>
class StructureDeltaDecompressorStream {
public:
    // Does not take ownership of the range.
    StructureDeltaDecompressorStream(const uint8_t* begin,
                                     const uint8_t* end)
        : stream_(begin, end) {
    }

    // Returns a reference to the latest decoded record (may not be
    // called if no records have been read yet). Valid only until next
    // call to next().
    const T& value() const {
        return value_;
    }

    bool empty() {
        return empty_;
    }

    // Reads a record from the input byte array. Returns false iff
    // no more records can be read. Once this returns false, the only
    // method that may be called on this object is empty().
    bool next() {
        if (!stream_.unpack(value_.bytes())) {
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

    T value_;
    bool empty_ = false;
    ByteArrayDeltaDecompressor<T::width_bytes(), Decompress> stream_;
};


#endif // COMPRESS_H

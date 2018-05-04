// -*- mode: c++ -*-

#ifndef BIT_PACKER_H
#define BIT_PACKER_H

template<size_t Bits>
struct Packer {
    static const int Bytes = (Bits + 7) / 8;

    Packer() {
    }

    template<typename T>
    size_t deposit(T data, size_t width, size_t at) {
        while (width) {
            size_t offset = (at % 8);
            int bits_to_deposit = std::min(width, 8 - offset);
            int deposit_at = at / 8;
            bytes_[deposit_at] |= data << offset;
            data >>= bits_to_deposit;
            at += bits_to_deposit;
            width -= bits_to_deposit;
        }

        return at;
    }

    template<typename T>
    uint64_t extract(T& data, size_t width, size_t at) const {
        T out = 0;
        size_t out_offset = 0;
        while (width) {
            int extract_from = at / 8;
            size_t offset = (at % 8);
            size_t bits_to_extract = std::min(width, 8 - offset);
            T extracted =
                (bytes_[extract_from] >> offset) &
                ((1 << bits_to_extract) - 1);
            out |= extracted << out_offset;
            out_offset += bits_to_extract;
            at += bits_to_extract;
            width -= bits_to_extract;
        }

        data = out;
        return at;
    }

    uint8_t bytes_[Bytes] = { 0 };
};

#endif // BIT_PACKER_H

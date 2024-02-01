#ifndef __BLOOMFILTER_H
#define __BLOOMFILTER_H

#include <cstddef>
#include <cstring>

#include "MurmurHash3.h"

namespace bloomfilter {
    template <typename T>
    struct BloomFilter {
        char *data = nullptr;
        const size_t size;

        BloomFilter(const size_t &size) : size(size) {
            this->data = new char[size];
            memset(this->data, 0, sizeof(size));
        }

        ~BloomFilter() { delete[] data; }

        void insert(const T &key) {
            int32_t x[4];
            MurmurHash3_x64_128(reinterpret_cast<const void *>(&key), sizeof(T), 1, x);
            for (int i = 0; i < 4; i++) {
                data[x[i] % this->size] = 1;
            }
        }

        bool check(const T &key) {
            int32_t x[4];
            MurmurHash3_x64_128(reinterpret_cast<const void *>(&key), sizeof(T), 1, x);
            for (int i = 0; i < 4; i++) {
                if (data[x[i] % this->size] == 0)
                    return false;
            }
            return true;
        }
    };
};  // namespace bloomfilter

#endif

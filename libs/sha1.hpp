#ifndef SHA1_HPP
#define SHA1_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <array>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>

namespace SHA1 {

    // Left rotation (circular shift)
    inline uint32_t leftrotate(uint32_t value, int shift) {
        return (value << shift) | (value >> (32 - shift));
    }

    // Compute SHA-1 hash of a buffer
    inline std::array<uint8_t, 20> compute(const uint8_t *message, std::size_t len) {
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        // ---- Padding ----
        size_t new_len = len + 1;
        while ((new_len % 64) != 56) new_len++;

        uint8_t *buffer = static_cast<uint8_t*>(calloc(new_len + 8, 1));
        std::memcpy(buffer, message, len);
        buffer[len] = 0x80;

        uint64_t bit_len = static_cast<uint64_t>(len) * 8;
        for (int i = 0; i < 8; i++) {
            buffer[new_len + i] = (uint8_t)(bit_len >> (56 - 8 * i));
        }

        // ---- Process 512-bit blocks ----
        for (size_t offset = 0; offset < new_len + 8; offset += 64) {
            uint32_t w[80];

            // Break block into 16 big-endian words
            for (int i = 0; i < 16; i++) {
                w[i]  = (buffer[offset + i*4 + 0] << 24);
                w[i] |= (buffer[offset + i*4 + 1] << 16);
                w[i] |= (buffer[offset + i*4 + 2] << 8);
                w[i] |= (buffer[offset + i*4 + 3]);
            }

            // Extend to 80 words
            for (int i = 16; i < 80; i++) {
                w[i] = leftrotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
            }

            uint32_t a = h0;
            uint32_t b = h1;
            uint32_t c = h2;
            uint32_t d = h3;
            uint32_t e = h4;

            for (int i = 0; i < 80; i++) {
                uint32_t f, k;
                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                uint32_t temp = leftrotate(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = leftrotate(b, 30);
                b = a;
                a = temp;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        free(buffer);

        // ---- Produce final hash ----
        std::array<uint8_t, 20> hash{};
        hash[ 0] = (h0 >> 24) & 0xFF;
        hash[ 1] = (h0 >> 16) & 0xFF;
        hash[ 2] = (h0 >>  8) & 0xFF;
        hash[ 3] = (h0      ) & 0xFF;

        hash[ 4] = (h1 >> 24) & 0xFF;
        hash[ 5] = (h1 >> 16) & 0xFF;
        hash[ 6] = (h1 >>  8) & 0xFF;
        hash[ 7] = (h1      ) & 0xFF;

        hash[ 8] = (h2 >> 24) & 0xFF;
        hash[ 9] = (h2 >> 16) & 0xFF;
        hash[10] = (h2 >>  8) & 0xFF;
        hash[11] = (h2      ) & 0xFF;

        hash[12] = (h3 >> 24) & 0xFF;
        hash[13] = (h3 >> 16) & 0xFF;
        hash[14] = (h3 >>  8) & 0xFF;
        hash[15] = (h3      ) & 0xFF;

        hash[16] = (h4 >> 24) & 0xFF;
        hash[17] = (h4 >> 16) & 0xFF;
        hash[18] = (h4 >>  8) & 0xFF;
        hash[19] = (h4      ) & 0xFF;

        return hash;
    }

    // Convenience overload: compute from std::string
    inline std::array<uint8_t, 20> compute(const std::string &input) {
        return compute(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    }

    // Convert hash to hex string
    inline std::string toHexString(const std::array<uint8_t, 20> &hash) {
        std::ostringstream oss;
        for (auto b : hash) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        }
        return oss.str();
    }

} // namespace SHA1

#endif // SHA1_HPP

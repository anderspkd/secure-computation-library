/* SCL --- Secure Computation Library
 * Copyright (C) 2024 Anders Dalskov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "scl/util/sha256.h"

#include <algorithm>
#include <cstdint>

/**
 * SHA-256 implementation based on https://github.com/System-Glitch/SHA256.
 */

namespace {

auto rotR(uint32_t x, unsigned n) {
  return (x >> n) | (x << (32 - n));
}

auto sig0(uint32_t x) {
  return rotR(x, 7) ^ rotR(x, 18) ^ (x >> 3);
}

auto sig1(uint32_t x) {
  return rotR(x, 17) ^ rotR(x, 19) ^ (x >> 10);
}

auto split(std::array<unsigned char, 64>& chunk) {
  std::array<uint32_t, 64> split;
  for (std::size_t i = 0, j = 0; i < 16; ++i, j += 4) {
    split[i] = (chunk[j] << 24)        //
               | (chunk[j + 1] << 16)  //
               | (chunk[j + 2] << 8)   //
               | (chunk[j + 3]);
  }

  for (std::size_t i = 16; i < 64; ++i) {
    split[i] = sig1(split[i - 2]) + sig0(split[i - 15]);
    split[i] += split[i - 7] + split[i - 16];
  }

  return split;
}

auto majority(uint32_t x, uint32_t y, uint32_t z) {
  return (x & (y | z)) | (y & z);
}

auto choose(uint32_t x, uint32_t y, uint32_t z) {
  return (x & y) ^ (~x & z);
}

}  // namespace

void scl::util::Sha256::transform() {
  // round constants.
  static constexpr std::array<uint32_t, 64> k = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
      0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
      0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
      0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
      0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

  const auto m = split(m_chunk);
  auto s = m_state;

  for (std::size_t i = 0; i < 64; ++i) {
    const auto maj = majority(s[0], s[1], s[2]);
    const auto chs = choose(s[4], s[5], s[6]);

    const auto xor_a = rotR(s[0], 2) ^ rotR(s[0], 13) ^ rotR(s[0], 22);
    const auto xor_e = rotR(s[4], 6) ^ rotR(s[4], 11) ^ rotR(s[4], 25);

    const auto sum = m[i] + k[i] + s[7] + chs + xor_e;

    const auto new_a = xor_a + maj + sum;
    const auto new_e = s[3] + sum;

    s[7] = s[6];
    s[6] = s[5];
    s[5] = s[4];
    s[4] = new_e;
    s[3] = s[2];
    s[2] = s[1];
    s[1] = s[0];
    s[0] = new_a;
  }

  for (std::size_t i = 0; i < 8; ++i) {
    m_state[i] += s[i];
  }
}

void scl::util::Sha256::pad() {
  auto i = m_chunk_pos;
  const auto end = m_chunk_pos < 56U ? 56U : 64U;

  m_chunk[i++] = 0x80;
  while (i < end) {
    m_chunk[i++] = 0;
  }

  if (m_chunk_pos >= 56) {
    transform();
    std::fill(m_chunk.begin(), m_chunk.begin() + 56, 0);
  }

  m_total_len += static_cast<std::size_t>(m_chunk_pos) * 8;

  m_chunk[63] = m_total_len;
  m_chunk[62] = m_total_len >> 8;
  m_chunk[61] = m_total_len >> 16;
  m_chunk[60] = m_total_len >> 24;
  m_chunk[59] = m_total_len >> 32;
  m_chunk[58] = m_total_len >> 40;
  m_chunk[57] = m_total_len >> 48;
  m_chunk[56] = m_total_len >> 56;

  transform();
}

scl::util::Sha256::DigestType scl::util::Sha256::writeDigest() {
  Sha256::DigestType digest;

  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = 0; j < 8; ++j) {
      digest[i + (j * 4)] = (m_state[j] >> (24 - i * 8)) & 0xFF;
    }
  }

  return digest;
}

void scl::util::Sha256::hash(const unsigned char* bytes, std::size_t nbytes) {
  for (std::size_t i = 0; i < nbytes; ++i) {
    m_chunk[m_chunk_pos++] = bytes[i];
    if (m_chunk_pos == 64) {
      transform();
      m_total_len += 512;
      m_chunk_pos = 0;
    }
  }
}

scl::util::Sha256::DigestType scl::util::Sha256::write() {
  pad();
  return writeDigest();
}

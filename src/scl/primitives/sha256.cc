/**
 * @file sha256.cc
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#include "scl/primitives/sha256.h"

#include <cstdint>
#include <cstring>

/**
 * SHA-256 implementation based on https://github.com/System-Glitch/SHA256.
 */

namespace {

auto RotR(uint32_t x, unsigned n) {
  return (x >> n) | (x << (32 - n));
}

auto Sig0(uint32_t x) {
  return RotR(x, 7) ^ RotR(x, 18) ^ (x >> 3);
}

auto Sig1(uint32_t x) {
  return RotR(x, 17) ^ RotR(x, 19) ^ (x >> 10);
}

auto Split(std::array<unsigned char, 64>& chunk) {
  std::array<uint32_t, 64> split;
  for (std::size_t i = 0, j = 0; i < 16; ++i, j += 4) {
    split[i] = (chunk[j] << 24)        //
               | (chunk[j + 1] << 16)  //
               | (chunk[j + 2] << 8)   //
               | (chunk[j + 3]);
  }

  for (std::size_t i = 16; i < 64; ++i) {
    split[i] = Sig1(split[i - 2]) + Sig0(split[i - 15]);
    split[i] += split[i - 7] + split[i - 16];
  }

  return split;
}

auto Majority(uint32_t x, uint32_t y, uint32_t z) {
  return (x & (y | z)) | (y & z);
}

auto Choose(uint32_t x, uint32_t y, uint32_t z) {
  return (x & y) ^ (~x & z);
}

}  // namespace

void scl::details::Sha256::Transform() {
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

  const auto m = Split(mChunk);
  auto s = mState;

  for (std::size_t i = 0; i < 64; ++i) {
    const auto maj = Majority(s[0], s[1], s[2]);
    const auto chs = Choose(s[4], s[5], s[6]);

    const auto xor_a = RotR(s[0], 2) ^ RotR(s[0], 13) ^ RotR(s[0], 22);
    const auto xor_e = RotR(s[4], 6) ^ RotR(s[4], 11) ^ RotR(s[4], 25);

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
    mState[i] += s[i];
  }
}

void scl::details::Sha256::Pad() {
  auto i = mChunkPos;
  const auto end = mChunkPos < 56U ? 56U : 64U;

  mChunk[i++] = 0x80;
  while (i < end) {
    mChunk[i++] = 0;
  }

  if (mChunkPos >= 56) {
    Transform();
    std::fill(mChunk.begin(), mChunk.begin() + 56, 0);
  }

  mTotalLen += static_cast<std::size_t>(mChunkPos) * 8;

  mChunk[63] = mTotalLen;
  mChunk[62] = mTotalLen >> 8;
  mChunk[61] = mTotalLen >> 16;
  mChunk[60] = mTotalLen >> 24;
  mChunk[59] = mTotalLen >> 32;
  mChunk[58] = mTotalLen >> 40;
  mChunk[57] = mTotalLen >> 48;
  mChunk[56] = mTotalLen >> 56;

  Transform();
}

scl::details::Sha256::DigestType scl::details::Sha256::WriteDigest() {
  scl::details::Sha256::DigestType digest;

  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = 0; j < 8; ++j) {
      digest[i + (j * 4)] = (mState[j] >> (24 - i * 8)) & 0xFF;
    }
  }

  return digest;
}

void scl::details::Sha256::Hash(const unsigned char* bytes,
                                std::size_t nbytes) {
  for (std::size_t i = 0; i < nbytes; ++i) {
    mChunk[mChunkPos++] = bytes[i];
    if (mChunkPos == 64) {
      Transform();
      mTotalLen += 512;
      mChunkPos = 0;
    }
  }
}

scl::details::Sha256::DigestType scl::details::Sha256::Write() {
  Pad();
  return WriteDigest();
}

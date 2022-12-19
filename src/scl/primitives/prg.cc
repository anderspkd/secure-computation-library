/**
 * @file prg.cc
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

#include "scl/primitives/prg.h"

#include <cstring>
#include <stdexcept>

#include <emmintrin.h>

/**
 * PRG implementation based on AES-CTR with code from
 * https://github.com/sebastien-riou/aes-brute-force
 */

#define BLOCK_SIZE sizeof(__m128i)

#define DO_ENC_BLOCK(m, k)                  \
  do {                                      \
    (m) = _mm_xor_si128(m, (k)[0]);         \
    (m) = _mm_aesenc_si128(m, (k)[1]);      \
    (m) = _mm_aesenc_si128(m, (k)[2]);      \
    (m) = _mm_aesenc_si128(m, (k)[3]);      \
    (m) = _mm_aesenc_si128(m, (k)[4]);      \
    (m) = _mm_aesenc_si128(m, (k)[5]);      \
    (m) = _mm_aesenc_si128(m, (k)[6]);      \
    (m) = _mm_aesenc_si128(m, (k)[7]);      \
    (m) = _mm_aesenc_si128(m, (k)[8]);      \
    (m) = _mm_aesenc_si128(m, (k)[9]);      \
    (m) = _mm_aesenclast_si128(m, (k)[10]); \
  } while (0)

#define AES_128_KEY_EXP(k, rcon) \
  Aes128KeyExpansion(k, _mm_aeskeygenassist_si128(k, rcon))

namespace {

auto Aes128KeyExpansion(__m128i key, __m128i keygened) {
  keygened = _mm_shuffle_epi32(keygened, _MM_SHUFFLE(3, 3, 3, 3));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  return _mm_xor_si128(key, keygened);
}

void Aes128LoadKey(const unsigned char* enc_key, __m128i* key_schedule) {
  const auto* k = reinterpret_cast<const __m128i*>(enc_key);
  key_schedule[0] = _mm_loadu_si128(k);
  key_schedule[1] = AES_128_KEY_EXP(key_schedule[0], 0x01);
  key_schedule[2] = AES_128_KEY_EXP(key_schedule[1], 0x02);
  key_schedule[3] = AES_128_KEY_EXP(key_schedule[2], 0x04);
  key_schedule[4] = AES_128_KEY_EXP(key_schedule[3], 0x08);
  key_schedule[5] = AES_128_KEY_EXP(key_schedule[4], 0x10);
  key_schedule[6] = AES_128_KEY_EXP(key_schedule[5], 0x20);
  key_schedule[7] = AES_128_KEY_EXP(key_schedule[6], 0x40);
  key_schedule[8] = AES_128_KEY_EXP(key_schedule[7], 0x80);
  key_schedule[9] = AES_128_KEY_EXP(key_schedule[8], 0x1B);
  key_schedule[10] = AES_128_KEY_EXP(key_schedule[9], 0x36);
}

void Aes128Enc(const __m128i* key_schedule, __m128i m, unsigned char* ct) {
  DO_ENC_BLOCK(m, key_schedule);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(ct), m);
}

auto create_mask(long counter) {
  return _mm_set_epi64x(PRG_NONCE, counter);
}

}  // namespace

scl::PRG::PRG(const unsigned char* seed) {
  if (seed != nullptr) {
    std::copy(seed, seed + SeedSize(), mSeed.begin());
  }
  Init();
}

void scl::PRG::Update() {
  mCounter += 1;
}

void scl::PRG::Init() {
  Aes128LoadKey(mSeed.data(), mState);
}

void scl::PRG::Reset() {
  Init();
  mCounter = PRG_INITIAL_COUNTER;
}

void scl::PRG::Next(unsigned char* buffer, size_t n) {
  if (n == 0) {
    return;
  }

  auto nblocks = n / BLOCK_SIZE;

  if ((n % BLOCK_SIZE) != 0) {
    nblocks++;
  }

  auto mask = create_mask(mCounter);
  auto out = std::make_unique<unsigned char[]>(nblocks * BLOCK_SIZE);
  auto* p = out.get();
  for (size_t i = 0; i < nblocks; i++) {
    Aes128Enc(mState, mask, p);
    Update();
    mask = create_mask(mCounter);
    p += BLOCK_SIZE;
  }

  std::copy(out.get(), out.get() + n, buffer);
}

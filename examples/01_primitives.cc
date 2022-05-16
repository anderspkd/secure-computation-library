/**
 * @file 01_primitives.cc
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

#include <scl/scl.h>

int main() {
  /* SCL supports two types of primitives: Hash functions and PRGs. A hash
   * function can be instantiated quite simply by using the
   * scl::Hash<OutputSize> class. The template parameter determines the size of
   * the digest and must be one of either 256, 384 or 512.
   */
  scl::Hash<256> hash;

  /* scl::Hash uses the IUF interface. Update is possible with either a
   * pointer+size or an STL vector.
   */
  unsigned char* buf = (unsigned char*)"1111";
  hash.Update(buf, 5);

  /* Calling finalize returns a digest, which is an STL array of some size.
   */
  auto digest = hash.Finalize();
  static_assert(
      std::is_same<decltype(digest), std::array<unsigned char, 32>>::value);

  /* The DigestToString can be used to print a hex representation of a digest.
   */
  std::cout << scl::DigestToString(digest) << "\n";
}

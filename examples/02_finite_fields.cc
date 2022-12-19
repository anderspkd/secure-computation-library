/**
 * @file 02_finite_fields.cc
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

#include <iostream>

#include <scl/math.h>

int main() {
  /* This defines a "Finite Field" with space for at least 32 bits of
   * computation. At the moment, SCL supports two primes: One that is 61 bits
   * and one that is 127 bits. The below definition will use the smaller of the
   * two primes.
   *
   * It is possible to later retrieve the size provided by calling
   * FF::SpecifiedBitSize(). On the other hand, we can also call FF::BitSize()
   * which returns the actual size of a field element (so 61 bits in the below
   * case).
   */
  using Fp = scl::Fp<32>;

  /* FF supports constructing an element from an int constant. The value input
   * is interpreted "modulo p" where p is the prime. This makes it possible to
   * define useful constants like 1, -1 and so on.
   *
   * The default construtor of FF can be used to construct an element equal to
   * 0.
   */
  auto a = Fp(1);
  auto b = Fp(1234);
  auto c = Fp(555);

  /* FF supports all operations required for a field, so addition, subtraction,
   * multiplication and "division". Division is defined as multiplication by the
   * inverse of the dividend.
   *
   * Additionally, FF supports inplace operators such as += and -= as well.
   */
  auto x = a + b;
  auto y = b * c;

  std::cout << a << " + " << b << " = " << x << "\n";
  std::cout << b << " * " << c << " = " << y << "\n";

  auto w = a / b;
  std::cout << a << " / " << b << " = " << w << "\n";

  /* Equality checking works as well. Note that FF does not define an order, so
   * it is not possible to ask if one field element is "smaller" than another.
   */
  std::cout << a << " ?= " << b << ": " << (a == b) << "\n";
  std::cout << a << " ?= " << a << ": " << (a == a) << "\n";

  auto prg = scl::PRG::Create();

  /* Using a PRG (see the PRG example), we can generate random field elements.
   */
  std::cout << Fp::Random(prg) << "\n";
  std::cout << Fp::Random(prg) << "\n";
  std::cout << Fp::Random(prg) << "\n";

  /* Serialization is also supported.
   */
  unsigned char buffer[Fp::ByteSize()];

  a.Write(buffer);
  auto a_ = Fp::Read(buffer);

  std::cout << (a_ == a) << "\n";
}

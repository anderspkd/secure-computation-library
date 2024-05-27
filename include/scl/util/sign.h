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

#ifndef SCL_UTIL_SIGN_H
#define SCL_UTIL_SIGN_H

#include <memory>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/ff.h"

namespace scl::util {

/**
 * @brief A signature for some signature scheme.
 * @tparam SIGNATURE_SCHEME the signature scheme.
 */
template <typename SIGNATURE_SCHEME>
struct Signature;

class ECDSA;

/**
 * @brief An ECDSA signature.
 */
template <>
struct Signature<ECDSA> {
 private:
  using ElementType = math::FF<math::ec::Secp256k1::Scalar>;

 public:
  /**
   * @brief The size of an ECDSA signature in bytes.
   */
  constexpr static std::size_t byteSize() {
    return ElementType::byteSize() * 2;
  }

  /**
   * @brief Read an ECDSA signature from a buffer.
   * @param buf the buffer.
   * @return an ECDSA signature.
   */
  static Signature<ECDSA> read(const unsigned char* buf) {
    return {ElementType::read(buf),
            ElementType::read(buf + ElementType::byteSize())};
  }

  /**
   * @brief Write an ECDSA signature to a stream.
   * @param buf the buffer.
   */
  void write(unsigned char* buf) const {
    r.write(buf);
    s.write(buf + ElementType::byteSize());
  }

  /**
   * @brief The r part of an ECDSA signature.
   */
  ElementType r;

  /**
   * @brief The s part of an ECDSA signature.
   */
  ElementType s;
};

/**
 * @brief The ECDSA signature scheme.
 */
class ECDSA {
 public:
  /**
   * @brief Public key type. A curve point.
   */
  using PublicKey = math::EC<math::ec::Secp256k1>;

  /**
   * @brief Secret key type. An element modulo the order of the curve.
   */
  using SecretKey = PublicKey::ScalarField;

  /**
   * @brief Derive the public key correspond to a given secret key.
   * @param secret_key the secret key.
   * @return A public key.
   */
  static PublicKey derive(const SecretKey& secret_key) {
    return secret_key * PublicKey::generator();
  }

  /**
   * @brief Sign a message.
   * @tparam DIGEST a digest type.
   * @param secret_key the secret key for signing.
   * @param digest the digest to sign.
   * @param prg a PRG used to select the nonce in the signature.
   * @return an ECDSA signature.
   */
  template <typename DIGEST>
  static Signature<ECDSA> Sign(const SecretKey& secret_key,
                               const DIGEST& digest,
                               PRG& prg) {
    const auto k = SecretKey::random(prg);
    const auto R = k * PublicKey::generator();
    const auto rx = conversionFunc(R);
    const auto h = digestToElement(digest);

    return {rx, k.inverse() * (h + secret_key * rx)};
  }

  /**
   * @brief Verify a signature.
   * @param public_key the public key of the signer.
   * @param signature the signature to verify.
   * @param digest the digest that was signed.
   * @return true if the signature is valid and false otherwise.
   */
  template <typename DIGEST>
  static bool verify(const PublicKey& public_key,
                     const Signature<ECDSA>& signature,
                     const DIGEST& digest) {
    const auto h = digestToElement(digest);
    const auto [r, s] = signature;
    const auto si = s.inverse();
    const auto R1 = (h * si) * PublicKey::generator();
    const auto R2 = (r * si) * public_key;
    const auto R = R1 + R2;
    return !R.isPointAtInfinity() && conversionFunc(R) == r;
  }

  /**
   * @brief Computes the ECDSA conversion function.
   * @param R the curve point to convert into a scalar field element.
   * @return a scalar.
   *
   * This function computes the \f$C(R)\f$ function that takes curve point
   * \f$R=(r_x, r_y)\f$ and outputs a scalar as \f$r_x \mod p\f$ where \f$p\f$
   * is order of a subgroup.
   */
  static SecretKey conversionFunc(const PublicKey& R) {
    const auto rx_f = R.toAffine()[0];
    unsigned char rx_bytes[SecretKey::byteSize()];
    rx_f.write(rx_bytes);
    return SecretKey::read(rx_bytes);
  }

  /**
   * @brief Converts a digest into an element of the scalar field.
   * @param digest the digest.
   * @return a scalar.
   */
  template <typename DIGEST>
  static SecretKey digestToElement(const DIGEST& digest) {
    if (digest.size() < SecretKey::byteSize()) {
      unsigned char buf[SecretKey::byteSize()] = {0};
      std::copy(digest.begin(), digest.end(), buf);
      return SecretKey::read(buf);
    }
    return SecretKey::read(digest.data());
  }
};

}  // namespace scl::util

#endif  // SCL_UTIL_SIGN_H

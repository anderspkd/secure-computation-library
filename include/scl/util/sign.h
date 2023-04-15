/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
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
#include "scl/math/ff.h"

namespace scl::util {

/**
 * @brief A signature for some signature scheme.
 * @tparam SignatureScheme the signature scheme.
 */
template <typename SignatureScheme>
struct Signature;

class ECDSA;

/**
 * @brief An ECDSA signature.
 */
template <>
struct Signature<ECDSA> {
 private:
  using ElementType = math::FF<math::Secp256k1::Order>;

 public:
  /**
   * @brief The size of an ECDSA signature in bytes.
   */
  constexpr static std::size_t ByteSize() {
    return ElementType::ByteSize() * 2;
  }

  /**
   * @brief Read an ECDSA signature from a buffer.
   * @param buf the buffer.
   * @return an ECDSA signature.
   */
  static Signature<ECDSA> Read(const unsigned char* buf) {
    return {ElementType::Read(buf),
            ElementType::Read(buf + ElementType::ByteSize())};
  }

  /**
   * @brief Write an ECDSA signature to a stream.
   * @param buf the buffer.
   */
  void Write(unsigned char* buf) const {
    r.Write(buf);
    s.Write(buf + ElementType::ByteSize());
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
  using Curve = math::EC<math::Secp256k1>;

 public:
  /**
   * @brief Public key type. A curve point.
   */
  using PublicKey = Curve;

  /**
   * @brief Secret key type. An element modulo the order of the curve.
   */
  using SecretKey = Curve::Order;

  /**
   * @brief Derive the public key correspond to a given secret key.
   * @param secret_key the secret key.
   * @return A public key.
   */
  static PublicKey Derive(const SecretKey& secret_key) {
    return secret_key * PublicKey::Generator();
  }

  /**
   * @brief Sign a message.
   * @tparam D a digest type.
   * @param secret_key the secret key for signing.
   * @param digest the digest to sign.
   * @param prg a PRG used to select the nonce in the signature.
   * @return an ECDSA signature.
   */
  template <typename D>
  static Signature<ECDSA> Sign(const SecretKey& secret_key,
                               const D& digest,
                               PRG& prg) {
    const auto k = Curve::Order::Random(prg);
    const auto R = k * PublicKey::Generator();
    const auto rx = ConversionFunc(R);
    const auto h = DigestToElement<Curve::Order>(digest);

    return {rx, k.Inverse() * (h + secret_key * rx)};
  }

  /**
   * @brief Verify a signature.
   * @param public_key the public key of the signer.
   * @param signature the signature to verify.
   * @param digest the digest that was signed.
   * @return true if the signature is valid and false otherwise.
   */
  template <typename D>
  static bool Verify(const PublicKey& public_key,
                     const Signature<ECDSA>& signature,
                     const D& digest) {
    const auto h = DigestToElement<SecretKey>(digest);
    const auto [r, s] = signature;
    const auto si = s.Inverse();
    const auto R1 = (h * si) * Curve::Generator();
    const auto R2 = (r * si) * public_key;
    const auto R = R1 + R2;
    return !R.PointAtInfinity() && ConversionFunc(R) == r;
  }

 private:
  template <typename T, typename D>
  static T DigestToElement(const D& digest) {
    if (digest.size() < T::ByteSize()) {
      unsigned char buf[T::ByteSize()] = {0};
      std::copy(digest.begin(), digest.end(), buf);
      return T::Read(buf);
    }
    return T::Read(digest.data());
  }

  static Curve::Order ConversionFunc(const PublicKey& R) {
    const auto rx_f = R.ToAffine()[0];
    unsigned char rx_bytes[Curve::Field::ByteSize()];
    rx_f.Write(rx_bytes);
    return Curve::Order::Read(rx_bytes);
  }
};

}  // namespace scl::util

#endif  // SCL_UTIL_SIGN_H

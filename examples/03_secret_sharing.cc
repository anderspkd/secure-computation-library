/**
 * @file 03_secret_sharing.cc
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
#include <stdexcept>

#include <scl/math.h>
#include <scl/secret_sharing.h>

int main() {
  using Fp = scl::Fp<32>;
  using Vec = scl::Vec<Fp>;

  auto prg = scl::PRG::Create();

  /* We can easily create an additive secret sharing of some secret value:
   */
  Fp secret(12345);
  Vec shares = scl::CreateAdditiveShares(secret, 5, prg);

  std::cout << "additive shares:\n" << shares << "\n";

  /* We can of course also reconstruct the secret again, given all 5 shares:
   */
  auto reconstructed = scl::ReconstructAdditive(shares);
  std::cout << "secret: " << reconstructed << "\n";

  /* SCL comes with three different ways of creating secret-shares for an honest
   * majority: Without any checks, with error detection, and with error
   * correction. Lets see error detection at work first
   */

  auto factory =
      scl::ShamirSSFactory<Fp>::Create(1, prg, scl::SecurityLevel::CORRECT);
  /* We create 4 shamir shares with a threshold of 1.
   */
  auto shamir_shares = factory.Share(secret);
  std::cout << shamir_shares << "\n";

  /* Of course, these can be reconstructed. The second parameter is the
   * threshold. This performs reconstruction with error detection.
   */
  auto shamir_reconstructed =
      factory.Recover(shamir_shares, scl::SecurityLevel::DETECT);
  std::cout << shamir_reconstructed << "\n";

  /* If we introduce an error, then reconstruction fails
   */
  shamir_shares[2] = Fp(123);
  try {
    std::cout << factory.Recover(shamir_shares, scl::SecurityLevel::DETECT)
              << "\n";
  } catch (std::logic_error& e) {
    std::cout << e.what() << "\n";
  }

  /* On the other hand, we can use the robust reconstruction since the threshold
   * is low enough. I.e., because 4 >= 3*1 + 1.
   */
  auto r = factory.Recover(shamir_shares);
  std::cout << r << "\n";

  /* With a bit of extra work, we can even learn which share had the error.
   */

  /* first we need the alphas that were used when generating the shares. By
   * default these are just the field elements 1 through 4.
   */
  Vec alphas = {Fp(1), Fp(2), Fp(3), Fp(4)};
  auto pe = scl::details::ReconstructShamirRobust(shamir_shares, alphas, 1);

  /* pe is a pair of polynomials. The first is the original polynomial used for
   * generating the shares and the second is a polynomial whose roots tell which
   * share had errors.
   *
   * The secret is embedded in the constant term.
   */
  std::cout << std::get<0>(pe).Evaluate(Fp(0)) << "\n";

  /* This will be 0, indicating that the share corresponding to party 3 had an
   * error.
   */
  std::cout << std::get<1>(pe).Evaluate(Fp(3)) << "\n";

  /* Lastly, if there's too many errors, then correction is not possible
   */
  shamir_shares[1] = Fp(22);
  try {
    factory.Recover(shamir_shares);
  } catch (std::logic_error& e) {
    std::cout << e.what() << "\n";
  }
}

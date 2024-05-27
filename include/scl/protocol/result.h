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

#ifndef SCL_PROTOCOL_RESULT_H
#define SCL_PROTOCOL_RESULT_H

#include <any>
#include <memory>

namespace scl::proto {

struct Protocol;

/**
 * @brief The Result of running a Protocol.
 *
 * All Protocols must return a ProtocolResult object indicating (1) the next
 * protocol to run, and (2) the output produced by the protocol. Either can be
 * empty, which gives rise to the four constructions present on this class.
 */
struct ProtocolResult {
  /**
   * @brief Create a protocol result without any next steps or output.
   *
   * Result returned by a final protocol that produced no output.
   */
  static ProtocolResult done() {
    return {.next_protocol = nullptr, .result = {}};
  }

  /**
   * @brief Create a protocol result without any next steps and an output.
   *
   * Result returned by a final protocol that produced some output.
   */
  static ProtocolResult done(std::any output) {
    return ProtocolResult{.next_protocol = nullptr,
                          .result = std::move(output)};
  }

  /**
   * @brief Create a protocol result with a next step.
   *
   * Result returned by an intermediary protocol that produced no output.
   */
  static ProtocolResult next(std::unique_ptr<Protocol> next) {
    return ProtocolResult{.next_protocol = std::move(next), .result = {}};
  }

  /**
   * @brief Create a protocol result with a next step and output.
   *
   * Result returned by an intermediary protocol that produced some output.
   */
  static ProtocolResult next(std::unique_ptr<Protocol> next, std::any output) {
    return ProtocolResult{.next_protocol = std::move(next),
                          .result = std::move(output)};
  }

  /**
   * @brief The next protocol to run. A nullptr value indicates no next step.
   */
  std::unique_ptr<Protocol> next_protocol;

  /**
   * @brief The output of the protocol.
   */
  std::any result;
};

}  // namespace scl::proto

#endif  // SCL_PROTOCOL_RESULT_H

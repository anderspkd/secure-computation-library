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

#ifndef SCL_PROTOCOL_EVAL_H
#define SCL_PROTOCOL_EVAL_H

#include <any>
#include <memory>

#include "scl/coro/task.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/protocol/result.h"

namespace scl::proto {

/**
 * @brief Evaluate a protocol.
 * @param protocol the protocol to evaluate.
 * @param env the environment to use.
 * @param output_callback a callable that receives protocol outputs.
 *
 * This function will evaluate a protocol, passing any outputs that
 * the protocol produces to @p output_callback.
 */
template <typename CALLBACK>
coro::Task<void> evaluate(std::unique_ptr<Protocol> protocol,
                          Env& env,
                          CALLBACK output_callback) {
  while (protocol) {
    ProtocolResult result = co_await protocol->run(env);

    if (result.next_protocol) {
      protocol = std::move(result.next_protocol);
    }

    if (result.result.has_value()) {
      output_callback(result.result);
    }
  }

  co_return;
}

/**
 * @brief Evaluate a protocol.
 * @tparam RESULT the type of the protocol's output.
 * @param protocol the protocol to evaluate.
 * @param env the environment to use.
 *
 * This function evaluates a protocol and returns the result that it
 * produces. If the protocol terminates, but produces no output, then
 * an <code>std::logic_error</code> is thrown. Similarly, if the result produced
 * cannot be <code>std::any_cast</code> to something of type
 * <code>RESULT</code>, then an error is thrown.
 */
template <typename RESULT>
coro::Task<RESULT> evaluate(std::unique_ptr<Protocol> protocol, Env& env) {
  while (protocol) {
    ProtocolResult result = co_await protocol->run(env);

    if (result.next_protocol) {
      protocol = std::move(result.next_protocol);
    } else {
      if (result.result.has_value()) {
        co_return std::any_cast<RESULT>(result.result);
      } else {
        throw std::logic_error("Protocol did not produce any result");
      }
    }
  }
}

/**
 * @brief Evaluate a protocol that produces no result.
 * @param protocol the protocol to evaluate.
 * @param env the environment to use.
 */
template <>
inline coro::Task<void> evaluate(std::unique_ptr<Protocol> protocol, Env& env) {
  while (protocol) {
    ProtocolResult result = co_await protocol->run(env);
    protocol = std::move(result.next_protocol);
  }
}

}  // namespace scl::proto

#endif  // SCL_PROTOCOL_EVAL_H

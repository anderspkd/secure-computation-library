/**
 * @file simple.h
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

#ifndef _SCL_P_SIMPLE_H
#define _SCL_P_SIMPLE_H

#include <type_traits>

namespace scl {

/**
 * @brief An intermediary protocol step.
 */
template <typename T, typename Ctx>
struct ProtocolStep {
  /**
   * @brief Evaluate a step of the protocol.
   */
  auto Run(Ctx& context) { return static_cast<T*>(this)->Run(context); };
};

/**
 * @brief The final step of a protocol.
 */
template <typename T, typename Ctx>
struct LastProtocolStep {
  /**
   * @brief Finalize the protocol.
   */
  auto Finalize(Ctx& context) {
    return static_cast<T*>(this)->Finalize(context);
  };
};

namespace {
template <typename S, typename Ctx>
using IsStep =
    std::enable_if_t<std::is_base_of_v<ProtocolStep<S, Ctx>, S>, bool>;

template <typename S, typename Ctx>
using IsLastStep =
    std::enable_if_t<std::is_base_of_v<LastProtocolStep<S, Ctx>, S>, bool>;
}  // namespace

/**
 * @brief Recursively evaluate all internal steps of a protocol.
 */
template <typename S, typename Ctx, IsStep<S, Ctx> = true>
auto Evaluate(S& step, Ctx& context) {
  auto next = step.Run(context);
  return Evaluate(next, context);
}

/**
 * @brief Evaluate the last step of a protocol.
 */
template <typename S, typename Ctx, IsLastStep<S, Ctx> = true>
auto Evaluate(S& last, Ctx& context) {
  return last.Finalize(context);
}

}  // namespace scl

#endif  // _SCL_P_SIMPLE_H

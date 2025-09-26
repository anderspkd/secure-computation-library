/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#ifndef SCL_PROTOCOL_H
#define SCL_PROTOCOL_H

#include <any>

#include "scl/coro/task.h"
#include "scl/net/network.h"
#include "scl/time.h"

namespace scl {

struct Env {
  Network network;
  std::unique_ptr<Clock> clock;
};

struct Result;

struct Protocol {
  virtual ~Protocol() {}

  constexpr static const char* DEFAULT_NAME = "N/A";

  virtual Task<Result> run(Env& env) const = 0;

  virtual std::string name() const {
    return DEFAULT_NAME;
  }
};

struct Result {
  static Result done() {
    return Result{.next = nullptr, .output = {}};
  }

  static Result done(std::any output) {
    return Result{.next = nullptr, .output = output};
  }

  static Result nextStep(std::unique_ptr<Protocol> next) {
    return Result{.next = std::move(next), .output = {}};
  }

  static Result nextStep(std::unique_ptr<Protocol> next, std::any output) {
    return Result{.next = std::move(next), .output = output};
  }

  std::unique_ptr<Protocol> next;
  std::any output;
};

}  // namespace scl

#endif  // SCL_PROTOCOL_H

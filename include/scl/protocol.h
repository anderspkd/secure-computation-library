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

#pragma once

#include <any>
#include <stdexcept>
#include <type_traits>

#include "scl/coro/task.h"
#include "scl/net/network.h"
#include "scl/time.h"

namespace scl {

/**
 * @brief Protocol environment.
 * @ingroup eval
 *
 * Env is the interface with which a Protocol can interact with the outside
 * world, as it were. It contains (1) a way to send and receive data to the
 * other parties, and (2) a way to see how long the protocol has been running
 * for.
 */
struct Env {
  /**
   * @brief The network.
   */
  Network network;

  /**
   * @brief A clock.
   */
  std::unique_ptr<Clock> clock;
};

struct Result;

/**
 * @brief Protocol interface.
 * @ingroup eval
 *
 * Protocol defines the "smallest possible" protocol. Namely a protocol which
 * can be run once, and which might produce (1) some output, and (2) another
 * protocol which can then be run next. This allows us to string multiple
 * Protocols together in order to get a much larger protocol. For example,
 * instead of implementing a full evaluation of a circuit, it might be easier to
 * implement something which only evaluates a single layer of the circuit. To
 * evaluate the full circuit, we just need stich enough of these single-layer
 * protocols together.
 *
 * The following simple example illustrates this idea.
 * @code
 * #include <scl/protocol.h>
 *
 * class CountdownProtocol final : public Protocol {
 *  public:
 *   CountdownProtocol(int current) : m_current(current) {}

 *   Task<Result> run(Env& ignored) const override {
 *     if (m_current == 0) {
 *       co_return Result::done();
 *     } else {
 *       co_return Result::nextStep(
 *           std::make_unique<CountdownProtocol>(m_current - 1),
 *            m_current);
 *     }
 *   }

 *  private:
 *   int m_current;
 * };
 * @endcode
 */
struct Protocol {
  virtual ~Protocol() {}

  /**
   * @brief Default name of a protocol.
   */
  constexpr static const char* DEFAULT_NAME = "N/A";

  /**
   * @brief Runs the protocol.
   */
  virtual Task<Result> run(Env& env) const = 0;

  /**
   * @brief Returns the name of the protocol.
   */
  virtual std::string name() const {
    return DEFAULT_NAME;
  }
};

/**
 * @brief The result of calling Protocol::run.
 * @ingroup eval
 */
struct Result {
  /**
   * @brief Creates a Result with no next step, and no output.
   */
  static Result done() {
    return Result{.next = nullptr, .output = {}};
  }

  /**
   * @brief Creates a Result with no next step, and some output.
   */
  static Result done(std::any output) {
    return Result{.next = nullptr, .output = output};
  }

  /**
   * @brief Creates a Result with a next step, and no output.
   */
  static Result nextStep(std::unique_ptr<Protocol> next) {
    return Result{.next = std::move(next), .output = {}};
  }

  /**
   * @brief Creates a Result with a next step, and some output.
   */
  static Result nextStep(std::unique_ptr<Protocol> next, std::any output) {
    return Result{.next = std::move(next), .output = output};
  }

  std::unique_ptr<Protocol> next;
  std::any output;
};

/**
 * @brief Run a protocol.
 */
template <typename CALLBACK>
Task<void> runProtocol(std::unique_ptr<Protocol> protocol,
                       Env& env,
                       CALLBACK output_cb) {
  while (protocol) {
    Result result = co_await protocol->run(env);

    protocol = std::move(result.next);

    if (result.output.has_value()) {
      output_cb(result.output);
    }
  }
}

template <typename R>
Task<R> runProtocol(std::unique_ptr<Protocol> protocol, Env& env) {
  Result result;

  while (protocol) {
    result = co_await protocol->run(env);

    protocol = std::move(result.next);
  }

  if constexpr (!std::is_void_v<R>) {
    if (result.output.has_value()) {
      co_return std::any_cast<R>(result.output);
    } else {
      throw std::runtime_error("protocol never gave output");
    }
  }
}

}  // namespace scl

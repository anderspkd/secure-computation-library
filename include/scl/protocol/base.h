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

#ifndef SCL_PROTOCOL_BASE_H
#define SCL_PROTOCOL_BASE_H

#include <any>
#include <chrono>
#include <memory>
#include <thread>

#include "scl/net/network.h"
#include "scl/protocol/env.h"

namespace scl::proto {

/**
 * @brief Base class for protocols.
 *
 * Protocol provides a simple interface for any protocol that produce
 * output. Protocol defines only two methods.
 *
 * <p>Protocol::Run, which takes a Network (so that the protocol can
 * communicate). The output of Protocol::Run is either another protocol or
 * <code>nullptr</code>. in the former case, the return value represents the
 * next logical step of the protocol, while the latter case indicates that the
 * protocol has terminated.
 *
 * <p>Protocol::Output which returns the output of the protocol. The output is
 * something of type <code>std::any</code> and so can essentially be any
 * object. It is the user's job to know what the concrete type of the output is.
 */
struct Protocol {
  /**
   * @brief Default protocol name.
   */
  constexpr static const char* kDefaultName = "UNNAMED";

  virtual ~Protocol(){};
  /**
   * @brief Run the protocol.
   * @param env the protocol environment.
   * @return next protocol to run, or <code>nullptr</code> if we're done.
   */
  virtual std::unique_ptr<Protocol> Run(ProtocolEnvironment& env) = 0;

  /**
   * @brief A name for this protocol.
   * @return the protocol name.
   *
   * Override this method to provide a unique name for a protocol. The name
   * serves as a way to distinguish two Protocol implementations from each
   * other. The default value is Protocol::kDefaultName.
   */
  virtual std::string Name() const {
    return Protocol::kDefaultName;
  }

  /**
   * @brief Output produced by running the protocol.
   * @return the output.
   */
  virtual std::any Output() const {
    return {};
  }
};

/**
 * @brief Evaluate a protocol.
 * @param protocol the protocol.
 * @param output_cb a callback for consuming protocol output.
 * @param env the protocol environment.
 */
template <typename Callback>
void Evaluate(std::unique_ptr<Protocol> protocol,
              Callback output_cb,
              Env& env) {
  std::shared_ptr<Protocol> next = std::move(protocol);
  std::shared_ptr<Protocol> prev = next;

  while (next != nullptr) {
    next = next->Run(env);
    if (prev->Output().has_value()) {
      output_cb(prev->Output());
    }
    prev = next;
  }
}

/**
 * @brief Evaluate a protocol.
 * @param protocol the protocol.
 * @param network the network to evaluate the protocol with.
 * @param output_cb a callback for consuming protocol output.
 */
template <typename Callback>
void Evaluate(std::unique_ptr<Protocol> protocol,
              net::Network& network,
              Callback output_cb) {
  Env ctx{network,
          std::make_unique<RealTimeClock>(),
          std::make_unique<StlThreadContext>()};
  Evaluate(std::move(protocol), output_cb, ctx);
}

/**
 * @brief Evalate a protocol, discarding all outputs generated.
 * @param protocol the protocol to evaluate.
 * @param network the network to use.
 */
inline void Evaluate(std::unique_ptr<Protocol> protocol,
                     net::Network& network) {
  const auto sink = [](auto output) { (void)output; };
  Evaluate(std::move(protocol), network, sink);
}

}  // namespace scl::proto

#endif  // SCL_PROTOCOL_BASE_H

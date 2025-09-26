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

#ifndef SCL_SIMULATION_SIMULATOR_H
#define SCL_SIMULATION_SIMULATOR_H

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

#include "scl/protocol.h"
#include "scl/simulation/event.h"
#include "scl/simulation/network_description.h"

namespace scl {

/**
 * @brief Concept for protocol builder objects.
 *
 * A "ProtocolBuilder" is any type which, when invoked, returns an std::vector
 * of Protocol objects.
 */
template <typename BUILDER>
concept ProtocolBuilder = requires(BUILDER builder) {
  { builder() } -> std::convertible_to<std::vector<std::unique_ptr<Protocol>>>;
};

/**
 * @brief The simulator.
 */
class Simulator final {
 public:
  struct SimulationHook {
    using HookType = std::function<void(std::size_t, Event*)>;

    std::optional<EventType> trigger;
    HookType hook;
  };

  template <ProtocolBuilder BUILDER>
  void run(BUILDER& builder, NetworkDescription network_desc) {
    auto protocol = builder();
    if (protocol.size() != network_desc.size()) {
      throw std::logic_error("protocols do not match network definition");
    }
    run(std::move(protocol), network_desc);
  }

  template <typename HOOK>
    requires(std::convertible_to<HOOK, SimulationHook::HookType>)
  void addHook(EventType trigger, HOOK hook) {
    m_hooks.emplace_back(trigger, hook);
  }

  template <typename HOOK>
    requires(std::convertible_to<HOOK, SimulationHook::HookType>)
  void addHook(HOOK hook) {
    m_hooks.emplace_back({}, hook);
  }

 private:
  std::vector<SimulationHook> m_hooks;

  void run(std::vector<std::unique_ptr<Protocol>>&& protocols,
           NetworkDescription network_desc);
};

}  // namespace scl

#endif  // SCL_SIMULATION_SIMULATOR_H

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
  /**
   * @brief A simulation hook.
   *
   * Simulation hooks are functions which are run whenever a specific/any Event
   * is created by some party. Hooks enable a programmable way of gaining
   * insight into a running simulation.
   *
   * The following illustrates how it is possible to define a hook which
   * prints a message every time a party enters a Protocol.
   *
   * @code
   * void printHook(std::size_t pid, Event* event) {
   *   BeginEvent* be = (BeginEvent*)event;
   *   std::cout << "party " << pid << " started " << be->name() << "\n";
   * }
   *
   * // make sure to add the hook with the right trigger, otherwise we're
   * // gonna have a bad time.
   * sim.addHook(EventType::BEGIN_EVENT, printHook);
   * @endcode
   */
  struct SimulationHook {
    /**
     * @brief Type definition of hook functions.
     */
    using HookType = std::function<void(std::size_t, Event*)>;

    /**
     * @brief The trigger, if any.
     */
    std::optional<EventType> trigger;

    /**
     * @brief The hook function.
     */
    HookType hook;
  };

  /**
   * @brief Run the simulation.
   */
  template <ProtocolBuilder BUILDER>
  void run(BUILDER& builder, NetworkDescription network_desc) {
    auto protocol = builder();
    if (protocol.size() != network_desc.size()) {
      throw std::logic_error("protocols do not match network definition");
    }
    run(std::move(protocol), network_desc);
  }

  /**
   * @brief Add a hook with a trigger.
   */
  template <typename HOOK>
    requires(std::convertible_to<HOOK, SimulationHook::HookType>)
  void addHook(EventType trigger, HOOK hook) {
    m_hooks.emplace_back(trigger, hook);
  }

  /**
   * @brief Add a hook without a trigger.
   */
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

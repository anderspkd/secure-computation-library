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

#ifndef SCL_SIMULATION_HOOK_H
#define SCL_SIMULATION_HOOK_H

#include <cstddef>

#include "scl/simulation/event.h"

namespace scl::sim {

class SimulationContext;

/**
 * @brief Interface for hooks.
 *
 * A hook is a piece of a code that is run in response to an event, and can
 * therefore be used to add custom logging or simulation termination.
 *
 * @code
 * struct MyHook final : public Hook {
 *   void run(std::size_t party_id, const ReadOnlyGlobalContext& ctx) override
 *   {
 *     auto event = (ProtocolEvent*)ctx.trace(party_id)->back();
 *     std::cout << "Party " << party_id
 *               << " finished running " event->protocol_name;
 *               << std::endl;
 *   }
 * };
 *
 * // elsewhere
 *
 * Manager* manager = // create a Manager object
 * manager->addHook<MyHook>(sim::EventType::PROTOCOL_END);
 * @endcode
 *
 * The hooks are run right after the triggering event has been added to the
 * party's event trace. It is therefore safe to assume that
 * <code>ctx.traces[party_id]</code> is not empty.
 *
 * A party, or the simulation as a whole, can be stopped through the
 * <code>SimulationContext</code> object that the hook receives. This is useful
 * to e.g., terminate the simulation when a particular party finishes.
 *
 * @code
 * struct MyHook final : public Hook {
 *   void run(std::size_t party_id, const ReadOnlyGlobalContext& ctx) override {
 *     // stop the other party
 *     ctx.cancel(1 - party);
 *   }
 * };
 *
 * // elsewhere
 *
 * Manager* manager = // ...
 * // call the hook when a party finishes the simulation. The hook will then
 * // cancel the other party, which must still be running.
 * manager->addHook<MyHook>(sim::EventType::STOP);
 * @endcode
 *
 * Terminating the calling party (the party indicated by the
 * <code>party_id</code> argument) on any of the following events
 * <ul>
 *  <li> <code>sim::EventType::STOP</code>
 *  <li> <code>sim::EventType::KILLED</code>
 *  <li> <code>sim::EventType::CANCELLED</code>
 * </ul>
 * is undefined behaviour.
 *
 * @see sim::Manager::addHook
 */
struct Hook {
  virtual ~Hook() {}

  /**
   * @brief Function to run.
   */
  virtual void run(std::size_t party_id, const SimulationContext& ctx) = 0;
};

/**
 * @brief A hook and trigger event.
 */
struct TriggerAndHook {
  /**
   * @brief The event to trigger the hook on.
   */
  std::optional<EventType> trigger;
  /**
   * @brief The hook.
   */
  std::unique_ptr<Hook> hook;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_HOOK_H

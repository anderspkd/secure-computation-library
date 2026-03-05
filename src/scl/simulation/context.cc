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

#include "scl/simulation/context.h"

#include "scl/simulation/event.h"
#include "scl/simulation/params.h"

using namespace scl;

details::SimulatorContext details::SimulatorContext::create(
    std::size_t number_of_parties,
    NetworkParams network_params,
    std::vector<Simulator::SimulationHook>&& hooks,
    std::optional<Simulator::OutputHandler>&& output_handler) {
  SimulatorContext ctx{network_params};

  ctx.m_number_of_parties = number_of_parties;
  ctx.m_events.resize(number_of_parties);
  ctx.m_clocks.resize(number_of_parties);
  ctx.m_hooks = std::move(hooks);
  ctx.m_output_handler = std::move(output_handler);
  ctx.m_cancellation_map.resize(number_of_parties, false);

  return ctx;
}

void details::SimulatorContext::runHooks(std::size_t pid, Event* event) {
  for (auto& [trigger, hook] : m_hooks) {
    // if the trigger was specified, then the hook is only run if the current
    // event matches the trigger type.
    if (trigger.has_value()) {
      if (trigger.value() == event->type()) {
        hook(pid, event);
      }
    } else {
      // otherwise the hook is run unconditionally.
      hook(pid, event);
    }
  }
}

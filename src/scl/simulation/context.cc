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

scl::details::SimulatorContext scl::details::SimulatorContext::create(
    NetworkDescription network_desc,
    std::vector<Simulator::SimulationHook>&& hooks) {
  SimulatorContext ctx{network_desc};

  ctx.m_events.resize(network_desc.size());
  ctx.m_clocks.resize(network_desc.size());
  ctx.m_hooks = std::move(hooks);

  return ctx;
}

void scl::details::SimulatorContext::runHooks(std::size_t pid, Event* event) {
  for (auto& [trigger, hook] : m_hooks) {
    // if the trigger was specified, then the hook is only run if the current
    // event matches the trigger type.
    if (trigger.has_value() && (trigger.value() == event->type())) {
      hook(pid, event);
    } else {
      // otherwise the hook is run unconditionally.
      hook(pid, event);
    }
  }
}

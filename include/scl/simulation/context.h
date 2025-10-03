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

#include <cstddef>
#include <type_traits>

#include "scl/simulation/event.h"
#include "scl/simulation/network_description.h"
#include "scl/simulation/simulator.h"

namespace scl::details {

class Context;

/**
 * @brief Global context object for the simulator.
 */
class SimulatorContext final {
 public:
  /**
   * @brief Create a new simulation context object.
   */
  static SimulatorContext create(
      NetworkDescription network_desc,
      std::vector<Simulator::SimulationHook>&& hooks);

  /**
   * @brief Get the context of a particular party in the simulation.
   */
  Context getContext(std::size_t id);

  /**
   * @brief Add an event to
   */
  template <typename EVENT, typename... ARGS>
    requires(std::is_base_of_v<Event, EVENT>)
  void addEvent(std::size_t pid, ARGS... args) {
    m_events[pid].add<EVENT>(std::forward<ARGS>(args)...);
    Event* latest = m_events[pid].latest();

    // don't run hooks for TRANSIENT events
    if (latest->type() != EventType::TRANSIENT) {
      runHooks(pid, latest);
    }
  }

  void startClock(std::size_t id) {
    m_clocks[id] = Time::now();
  }

  Time::TimePoint readClock(std::size_t id) const {
    return m_clocks[id];
  }

  Event* lastEvent(std::size_t id) {
    return m_events[id].latest();
  }

  NetworkDescription::ChannelParameters getChannel(ChannelId cid) {
    return m_network_desc.getChannel(cid);
  }

  std::size_t numberOfParties() const {
    return m_network_desc.size();
  }

 private:
  NetworkDescription m_network_desc;
  std::vector<EventList> m_events;
  std::vector<Time::TimePoint> m_clocks;
  std::vector<Simulator::SimulationHook> m_hooks;

  SimulatorContext(NetworkDescription network_desc)
      : m_network_desc(network_desc) {}

  void runHooks(std::size_t pid, Event* event);
};

class Context {
 public:
  Event* lastEvent() {
    return m_ctx.lastEvent(m_id);
  }

  Time::Duration elapsedTimeOf(std::size_t id) const {
    return m_ctx.lastEvent(id)->time();
  }

  Time::Duration elapsedTime() const {
    return elapsedTimeOf(m_id) + (Time::now() - m_ctx.readClock(m_id));
  }

  void startClock() {
    m_ctx.startClock(m_id);
  }

  template <typename EVENT, typename... ARGS>
    requires(std::is_base_of_v<Event, EVENT>)
  void addEvent(ARGS... args) {
    m_ctx.addEvent<EVENT>(m_id, std::forward<ARGS>(args)...);
    startClock();
  }

 private:
  friend class SimulatorContext;

  SimulatorContext& m_ctx;
  std::size_t m_id;

  Context(SimulatorContext& ctx, std::size_t id) : m_ctx(ctx), m_id(id) {}
};

inline Context SimulatorContext::getContext(std::size_t id) {
  return Context{*this, id};
}

}  // namespace scl::details

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

#ifndef SCL_SIMULATION_CONTEXT_H
#define SCL_SIMULATION_CONTEXT_H

#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>

#include "scl/simulation/cancellation.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/hook.h"
#include "scl/util/bitmap.h"

namespace scl::sim {

class SimulationContext;

namespace details {

/**
 * @brief Global context object for a simulation.
 *
 * GlobalContext keeps track of the events that the parties in the simulation
 * generates, the timestamps of when a party sends data on a channel, and the
 * local clocks of each party.
 */
struct GlobalContext {
  /**
   * @brief Create a new global context for a simulation.
   * @param number_of_parties the number of parties in the simulation.
   * @param network_config the network configuration to use.
   * @param hooks the hooks that should be run when an event is created.
   */
  static GlobalContext create(std::size_t number_of_parties,
                              std::unique_ptr<NetworkConfig> network_config,
                              std::vector<TriggerAndHook> hooks);

  /**
   * @brief The number of parties.
   */
  std::size_t number_of_parties;

  /**
   * @brief The network configuration for the simulation.
   */
  std::unique_ptr<NetworkConfig> network_config;

  /**
   * @brief The simulation traces.
   */
  std::vector<SimulationTrace> traces;

  /**
   * @brief Current unhandled packets in the network.
   *
   * This is a mapping from a channel to timestamps of calls to send on the
   * channel that have not yet been received.
   */
  std::unordered_map<ChannelId, std::deque<util::Time::Duration>> sends;

  /**
   * @brief The local clocks for each party.
   */
  std::vector<util::Time::TimePoint> clocks;

  /**
   * @brief Map of parties currently in the process of receiving data.
   */
  std::vector<util::Bitmap> recv_map;

  /**
   * @brief Map used to indicate which parties have been stopped.
   */
  mutable util::Bitmap cancellation_map;

  /**
   * @brief Hooks.
   */
  std::vector<TriggerAndHook> hooks;

  /**
   * @brief A local version of a GlobalContext.
   *
   * LocalContext provides a local mutable "view" of the GlobalContext for a
   * particular party.
   */
  class LocalContext {
   public:
    /**
     * @brief Add an event to this party's simulation trace.
     * @param event the event.
     */
    void recordEvent(std::shared_ptr<Event> event);

    /**
     * @brief Indicate that this party is sending data to another party.
     * @param receiver the ID of the receiving party.
     * @param timestamp when the data was sent.
     */
    void send(std::size_t receiver, util::Time::Duration timestamp) {
      const ChannelId id{.local = m_id, .remote = receiver};
      m_gctx.sends[id].push_back(timestamp);
    }

    /**
     * @brief Receives an amount of bytes.
     * @param sender the ID of sending party.
     * @param nbytes the amount of bytes that this party wishes to receive.
     * @param timestamp the local time of the receiving party.
     * @return \p timestamp adjusted with an appropriate delay.
     *
     * <p>The return value is \p timestamp adjusted to account for any delay
     * that this party would incur in receiving \p nbytes.
     */
    util::Time::Duration recv(std::size_t sender,
                              std::size_t nbytes,
                              util::Time::Duration timestamp);

    /**
     * @brief Indicate that this party has started receiving data.
     */
    void recvStart(std::size_t id);

    /**
     * @brief Indicate that this party has stopped receiving data.
     */
    void recvDone(std::size_t id);

    /**
     * @brief Check of a party is in the process of receiving from us.
     */
    bool receiving(std::size_t receiver) const;

    /**
     * @brief Check if a party has terminated.
     */
    bool dead(std::size_t id) const;

    /**
     * @brief Returns the amount of elapsed so far.
     *
     * The amount of elapsed time is defined as the current running time
     * (defined as the timestamp on the last event produced by this party) plus
     * the time elapsed since the startClock was called.
     */
    util::Time::Duration elapsedTime() const;

    /**
     * @brief Get the current time of some other party in the protocol.
     * @param other_party the ID of the other party.
     */
    util::Time::Duration currentTimeOf(std::size_t other_party) const;

    /**
     * @brief Start the clock for this party.
     *
     * This internally sets the timestamp used to compute the elapsed time.
     * Thus, this function should be called whenever the party starts doing
     * "real work". E.g., just before a send or receive call on a simulated
     * channel returns.
     */
    void startClock();

    /**
     * @brief Get the timestamp of the most recent event.
     *
     * Does not check if an event exists.
     */
    util::Time::Duration lastEventTimestamp() const;

    /**
     * @brief Get a limited version of this context object.
     */
    SimulationContext getContext() const;

   private:
    friend struct GlobalContext;

    std::size_t m_id;
    GlobalContext& m_gctx;

    LocalContext(std::size_t id, GlobalContext& global)
        : m_id(id), m_gctx(global) {}
  };

  /**
   * @brief Get a local party's view of this context.
   * @param party_id the ID of the party.
   * @return a view of this context for party \p party_id.
   */
  LocalContext view(std::size_t party_id) {
    return LocalContext(party_id, *this);
  }
};

/**
 * @brief Output a global context object to a stream.
 */
std::ostream& operator<<(std::ostream& os, const GlobalContext& global_ctx);

}  // namespace details

/**
 * @brief A view of the current context object of the simulation.
 *
 * SimulationContext provides a view of the current simulation context with
 * minor options for mutability. This object is passed to a hook and allows
 * reacting when different events are produced.
 */
class SimulationContext {
 public:
  /**
   * @brief Get the trace of a particular party.
   */
  SimulationTrace trace(std::size_t party_id) const {
    return m_gctx.traces[party_id];
  }

  /**
   * @brief Get the running time of a party.
   */
  util::Time::Duration currentTimeOf(std::size_t party_id) const {
    return m_gctx.view(m_id).currentTimeOf(party_id);
  }

  /**
   * @brief Check if a party is still running, or if it is dead.
   */
  bool dead(std::size_t party_id) const {
    return m_gctx.view(m_id).dead(party_id);
  }

  /**
   * @brief Stop a party.
   */
  void cancel(std::size_t party_id) const {
    if (party_id != m_id) {
      m_gctx.cancellation_map.set(party_id, true);
    } else {
      throw details::CancellationException();
    }
  }

  /**
   * @brief Stop the simulation.
   */
  void cancelSimulation() const {
    for (std::size_t i = 0; i < m_gctx.number_of_parties; i++) {
      m_gctx.cancellation_map.set(i, true);
    }
    cancel(m_id);
  }

 private:
  friend class details::GlobalContext::LocalContext;

  std::size_t m_id;
  details::GlobalContext& m_gctx;

  SimulationContext(std::size_t id, details::GlobalContext& context)
      : m_id(id), m_gctx(context) {}
};

namespace details {

inline SimulationContext GlobalContext::LocalContext::getContext() const {
  return SimulationContext(m_id, m_gctx);
}

}  // namespace details

}  // namespace scl::sim

#endif  // SCL_SIMULATION_CONTEXT_H

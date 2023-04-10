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

#ifndef SCL_SIMULATION_CONTEXT_H
#define SCL_SIMULATION_CONTEXT_H

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>

#include "scl/simulation/buffer.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/mem_channel_buffer.h"

namespace scl::sim {

/**
 * @brief Context for simulations.
 */
class SimulationContext {
 private:
  enum class State { PREPARE, COMMIT, ROLLBACK };

 public:
  /**
   * @brief A write operation on the channel.
   */
  struct WriteOp {
    /**
     * @brief The amount of data written.
     */
    std::size_t amount;

    /**
     * @brief When the data was written.
     */
    util::Time::Duration time;
  };

  /**
   * @brief Create a new SimulationContext.
   * @tparam ChannelBufferT the type of the channel buffer to use
   * @param number_of_parties the number of parties in the simulation
   * @param config config for the simulated network
   * @return a pointer to a SimulationConfig.
   *
   * This factory method handles the non-trivial setup related to the network
   * buffers. A specialization exists for each of the ChannelBuffer
   * implementations that currently exist in SCL.
   */
  template <typename ChannelBufferT>
  static std::shared_ptr<SimulationContext> Create(
      std::size_t number_of_parties,
      const SimulatedNetworkConfigCreator& config);

  /**
   * @brief Create a new simulation context.
   * @param config a config describing the simulated network
   * @return a simulation context.
   *
   * This constructor simply sets the network config for the context but
   * otherwise performs no initialization whatsoever. Use Create instead.
   */
  SimulationContext(const SimulatedNetworkConfigCreator& config)
      : m_network_conf_creator(config), m_nparties(0) {}

  /**
   * @brief Get the network config for a particular channel.
   * @param channel_id the ID of the channel
   * @return a SimulatedNetworkConfig for the channel.
   */
  SimulatedNetworkConfig NetworkConfig(ChannelId channel_id) const {
    return m_network_conf_creator(channel_id);
  }

  /**
   * @brief Get the number of parties in the simulation.
   */
  std::size_t NumberOfParties() const {
    return m_nparties;
  }

  /**
   * @brief Get the channel buffer for a particular channel.
   * @param id the ID of the channel
   * @return the channel buffer.
   */
  std::shared_ptr<ChannelBuffer> Buffer(ChannelId id) {
    return m_buffers[id];
  }

  /**
   * @brief Record a write operation
   * @param id the ID of the channel the write was performed on
   * @param n the number of bytes written
   * @param ts a timestamp indicating when the write took place
   */
  void RecordWrite(ChannelId id, std::size_t n, util::Time::Duration ts) {
    m_writes[id].emplace_back(WriteOp{n, ts});
  }

  /**
   * @brief Get recorded write operations on a particular channel.
   * @param id the ID of the channel
   */
  std::vector<WriteOp>& Writes(ChannelId id) {
    return m_writes[id];
  }

  /**
   * @brief Add an event.
   * @param id the ID of the party adding the event
   * @param event the event
   */
  void AddEvent(std::size_t id, std::shared_ptr<Event> event) {
    m_traces[id].emplace_back(event);
  }

  /**
   * @brief Get all simulation traces.
   */
  std::vector<SimulationTrace> Trace() const {
    return m_traces;
  }

  /**
   * @brief Get the simulation trace of a particular party.
   */
  SimulationTrace Trace(std::size_t id) const {
    return m_traces[id];
  }

  /**
   * @brief Get the latest timestamp of a particular party.
   */
  util::Time::Duration LatestTimestamp(std::size_t id) const {
    return Trace(id).back()->Timestamp();
  }

  /**
   * @brief Find the ID of a suitable next party to run in the simulation.
   * @param current the last party to run
   * @return the ID of the next party to run, or none if the simulation is done.
   */
  std::optional<std::size_t> NextToRun(std::optional<std::size_t> current = {});

  /**
   * @brief Add a candidate party to run next.
   */
  void AddCandidateToRun(std::size_t id) {
    m_next_party_cand.emplace_back(id);
  };

  /**
   * @brief Update the checkpoint value to the current time.
   */
  void UpdateCheckpoint() {
    m_checkpoint = util::Time::Now();
  }

  /**
   * @brief Compute the time since the last time Checkpoint was called.
   */
  util::Time::Duration Checkpoint(std::size_t id);

  /**
   * @brief Get the value of the current checkpoint.
   */
  util::Time::TimePoint ReadCurrentCheckpoint() const {
    return m_checkpoint;
  }

  /**
   * @brief Prepare a party for running.
   */
  void Prepare(std::size_t id);

  /**
   * @brief Commit all the events and network data that a party generated.
   */
  void Commit(std::size_t id);

  /**
   * @brief Rollback changes that a party made.
   */
  void Rollback(std::size_t id);

 private:
  SimulatedNetworkConfigCreator m_network_conf_creator;

  std::size_t m_nparties;

  std::vector<SimulationTrace> m_traces;
  std::size_t m_trace_index;

  std::map<ChannelId, std::shared_ptr<ChannelBuffer>> m_buffers;

  State m_state = State::COMMIT;

  std::map<ChannelId, std::vector<WriteOp>> m_writes;
  std::map<ChannelId, std::vector<WriteOp>> m_writes_backup;

  util::Time::TimePoint m_checkpoint;

  std::vector<std::size_t> m_next_party_cand;
};

/**
 * @brief Create a simulation context with in-memory channels.
 */
template <>
std::shared_ptr<SimulationContext>
SimulationContext::Create<MemoryBackedChannelBuffer>(
    std::size_t number_of_parties,
    const SimulatedNetworkConfigCreator& config);

// template <>

}  // namespace scl::sim

#endif  // SCL_SIMULATION_CONTEXT_H

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

#include <memory>
#include <optional>
#include <queue>
#include <stdexcept>
#include <unordered_map>

#include "scl/simulation/buffer.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/mem_channel_buffer.h"

namespace scl::sim {

/**
 * @brief Context for simulations.
 */
class Context {
 private:
  enum class State { PREPARE, COMMIT, ROLLBACK };

 public:
  /**
   * @brief Provides a read-only view of a Context.
   */
  class View;

  /**
   * @brief A write operation on the channel.
   */
  struct WriteOp {
    /**
     * @brief Construct a new WriteOp.
     * @param amount the amount of data in the write operation.
     * @param time the time of the write operation.
     */
    WriteOp(std::size_t amount, util::Time::Duration time)
        : amount(amount), time(time) {}
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
  static std::shared_ptr<Context> Create(std::size_t number_of_parties,
                                         std::shared_ptr<NetworkConfig> config);

  /**
   * @brief Construct a new simulation context.
   * @param config a config describing the simulated network
   *
   * This constructor simply sets the network config for the context but
   * otherwise performs no initialization whatsoever. Use Create instead.
   */
  Context(std::shared_ptr<NetworkConfig> config)
      : m_network_config(config), m_nparties(0) {}

  /**
   * @brief Get the config for a channel.
   * @param channel_id the ID of the channel
   * @return a SimulatedNetworkConfig for the channel.
   */
  ChannelConfig ChannelConfiguration(ChannelId channel_id) const {
    return m_network_config->Get(channel_id);
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
   * @brief Add a write operation.
   * @param id the identifier of the channel that the write occured on.
   * @param n the number of bytes written.
   * @param time the time the write happened.
   */
  void AddWrite(ChannelId id, std::size_t n, util::Time::Duration time) {
    m_writes[id].emplace(n, time);
  }

  /**
   * @brief Check if a channel has any unprocessed writes on it.
   * @param id the identifier for the channel.
   * @return true if the channel has unprocessed writes. False otherwise.
   */
  bool HasWrite(ChannelId id) const {
    return !(m_writes.find(id) == m_writes.end() || m_writes.at(id).empty());
  }

  /**
   * @brief Get the next write on a channel.
   * @param id the identifier of the channel.
   * @return a write operation.
   *
   * This method does not check if there are any writes.
   */
  WriteOp& NextWrite(ChannelId id) {
    return m_writes[id].front();
  }

  /**
   * @brief Delete a write operation.
   * @param id the identifier of the channel.
   *
   * This method is meant to be called after a write operation has had all its
   * data processed. In a nutshell, when <code>op.amount == 0</code>.
   */
  void DeleteWrite(ChannelId id) {
    m_writes[id].pop();
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
   * @brief Check if a party has terminated.
   * @param id the ID of the party.
   * @return true if the party has terminated, and otherwise false.
   */
  bool HasTerminated(std::size_t id) const {
    if (Trace(id).empty()) {
      return false;
    }
    const auto t = Trace(id).back()->EventType();
    return t == sim::Event::Type::STOP || t == sim::Event::Type::KILLED;
  }

  /**
   * @brief Remove and return the last event added by a party.
   */
  std::shared_ptr<Event> PopLastEvent(std::size_t id) {
    auto evt = m_traces[id].back();
    m_traces[id].pop_back();
    return evt;
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

  /**
   * @brief Obtain a View of this context.
   */
  View GetView();

 private:
  std::shared_ptr<NetworkConfig> m_network_config;

  std::size_t m_nparties;

  std::vector<SimulationTrace> m_traces;
  std::size_t m_trace_index;

  std::unordered_map<ChannelId, std::shared_ptr<ChannelBuffer>> m_buffers;

  State m_state = State::COMMIT;

  std::unordered_map<ChannelId, std::queue<WriteOp>> m_writes;
  std::unordered_map<ChannelId, std::queue<WriteOp>> m_writes_backup;

  util::Time::TimePoint m_checkpoint;

  std::vector<std::size_t> m_next_party_cand;
};

/**
 * @brief Create a simulation context with in-memory channels.
 */
template <>
std::shared_ptr<Context> Context::Create<MemoryBackedChannelBuffer>(
    std::size_t number_of_parties,
    std::shared_ptr<NetworkConfig> config);

/**
 * @brief View of a context.
 *
 * View provides a read-only view of certain parts of the current Context.
 */
class Context::View {
 public:
  /**
   * @brief Get the trace of a party.
   * @param id the ID of the party.
   */
  SimulationTrace Trace(std::size_t id) const {
    return m_ctx.Trace(id);
  }

  /**
   * @brief Check if a party has terminated.
   * @param id the ID of the party.
   * @return true if the party has terminated, and otherwise false.
   */
  bool HasTerminated(std::size_t id) const {
    return m_ctx.HasTerminated(id);
  }

  /**
   * @brief Get the total number of parties in the simulation.
   */
  std::size_t NumberOfParties() const {
    return m_ctx.NumberOfParties();
  }

 private:
  friend Context;

  View(const Context& ctx) : m_ctx(ctx) {}

  const Context& m_ctx;
};

inline Context::View Context::GetView() {
  return Context::View(*this);
}

}  // namespace scl::sim

#endif  // SCL_SIMULATION_CONTEXT_H

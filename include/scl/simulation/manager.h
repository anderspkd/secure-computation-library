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

#ifndef SCL_SIMULATION_MANAGER_H
#define SCL_SIMULATION_MANAGER_H

#include <any>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "scl/protocol/base.h"
#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/hook.h"

namespace scl::sim {

/**
 * @brief Manager for a simulation.
 *
 * A Manager manages certain aspects of a protocol simulation:
 * <ul>
 * <li>The number of replications in the simulation.
 * <li>The protocol to simulate.
 * <li>What to do with the protocol(s) output.
 * <li>What network to use.
 * <li>When to terminate protocol(s).
 * <li>What to do when a protocol finishes.
 * </ul>
 *
 * Manager only requires implementing the Manager::protocol function that is
 * responsible for constructing the protocols to be simulated. Everything else
 * has sensible defaults.
 *
 * <h3>The Manager::protocol function</h3>
 *
 * This is one of two required function and specifies which protocol to
 * simulate. The return value is an STL vector of proto::Protocol objects to
 * simulate. The length of this vector is assumed by the simulator to define the
 * number of parties present in the protocol. The vector is allowed to contain
 * <code>nullptr</code> values. (These will simply correspond to parties that
 * are not running any code.)
 *
 * <h3>Handling simulation outputs</h3>
 *
 * The other of the required functions. Each run of the simulator produces a
 * list of traces (one per party). The Manager::handleSimulatorOutput function
 * decides what to do with said traces.
 *
 * <h3>Hooks</h3>
 *
 * Manager::addHook makes it possible to specify "hooks" that the simulator will
 * run before and after a protocols' proto::Protocol::run function is
 * called. Each hook is called with the ID of the protocol, corresponding to the
 * protocol's index in the vector that Manager::protocol returned, as well as a
 * "read-only" view of the simulators context.
 *
 * <h3>Handling protocol outputs</h3>
 *
 * Any output produced by a protocol will be passed to Manager::handleOutput,
 * and customizing this function therefore allows us to e.g., check correctness
 * of a protocol.
 */
class Manager {
 public:
  /**
   * @brief Destructor.
   */
  virtual ~Manager() {}

  /**
   * @brief Return a fresh instance of the protocol to simulate.
   *
   * Each simulation replication requires a <i>fresh</i> protocol instance to
   * run. This function takes care of returning such a protocol. The simulator
   * is assumed to take complete ownership over the returned protocol, so it is
   * important that objects returned by this function are independent of objects
   * previously returned by calling this function.
   */
  virtual std::vector<std::unique_ptr<proto::Protocol>> protocol() = 0;

  /**
   * @brief Handle the output of a simulation.
   * @param party_id the ID of the party that ran in the simulation.
   * @param trace the simulation trace produced by the simulator.
   */
  virtual void handleSimulatorOutput(std::size_t party_id,
                                     const SimulationTrace& trace) = 0;

  /**
   * @brief Handle the output produced by some party.
   * @param party_id the ID of the party who produced the output.
   * @param output the output.
   *
   * The default implementation simply discards the output.
   */
  virtual void handleProtocolOutput(std::size_t party_id,
                                    const std::any& output) {
    (void)party_id;
    (void)output;
  }

  /**
   * @brief Get the configuration for the network.
   *
   * The default is to return a SimpleNetworkConfig instance.
   */
  virtual std::unique_ptr<NetworkConfig> networkConfiguration() const {
    return std::make_unique<SimpleNetworkConfig>();
  }

  /**
   * @brief Add a new hook.
   * @tparam HOOK the hook.
   * @tparam HOOK_ARGS argument pack for the arguments passed to the hook.
   * @param trigger the event type to trigger the hook on.
   * @param args arguments to pass to the constructor of the hook.
   *
   * Use this function to add <code>sim::Hook</code>s to the simulation. The
   * hook to add is specified by the \p HOOK template argument, and the hook is
   * constructed by the addHook function in a manner similar to how
   * std::make_unique works. The added hook will be run every time an event of
   * type \p trigger is generated.
   */
  template <typename HOOK, typename... HOOK_ARGS>
  void addHook(EventType trigger, HOOK_ARGS&&... args) {
    static_assert(std::is_base_of_v<Hook, HOOK>);
    m_hooks.emplace_back(TriggerAndHook{
        trigger,
        std::make_unique<HOOK>(std::forward<HOOK_ARGS>(args)...)});
  }

  /**
   * @brief Add a new hook.
   * @tparam HOOK the hook.
   * @tparam HOOK_ARGS argument pack for the arguments passed to the hook.
   * @param args arguments to pass to the constructor of the hook.
   *
   * Use this function to add <code>sim::Hook</code>s to the simulation. The
   * hook to add is specified by the \p HOOK template argument, and the hook is
   * constructed by the addHook function in a manner similar to how
   * std::make_unique works. The added hook will be run for all events.
   */
  template <typename HOOK, typename... HOOK_ARGS>
  void addHook(HOOK_ARGS&&... args) {
    static_assert(std::is_base_of_v<Hook, HOOK>);
    m_hooks.emplace_back(TriggerAndHook{
        {},
        std::make_unique<HOOK>(std::forward<HOOK_ARGS>(args)...)});
  }

 private:
  friend void simulate(std::unique_ptr<sim::Manager> manager);
  std::vector<TriggerAndHook> m_hooks;
};

/**
 * @brief Manager that outputs traces to a stream.
 *
 * Writes simulation traces to a provided stream as a json object of the form:
 *
 * @code
 * {
 *   "replication": <replication>,
 *   "party_id": <party_id>,
 *   "trace": <trace>
 * }
 * @endcode
 */
class ManagerWithOutputToStream : public Manager {
 public:
  /**
   * @brief Create a new ManagerWithOutputToStream.
   * @param stream the stream to write the output to.
   */
  ManagerWithOutputToStream(std::ostream& stream) : m_stream(stream) {}

  void handleSimulatorOutput(std::size_t party_id,
                             const SimulationTrace& trace) override {
    m_stream << "{";
    m_stream << "\"party_id\":" << party_id << ",";
    m_stream << "\"trace\":";
    writeTrace(m_stream, trace);
    m_stream << "}" << std::endl;
  }

 private:
  std::ostream& m_stream;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_MANAGER_H

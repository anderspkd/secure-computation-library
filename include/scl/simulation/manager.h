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

#ifndef SCL_SIMULATION_MANAGER_H
#define SCL_SIMULATION_MANAGER_H

#include <any>
#include <cstddef>
#include <stdexcept>

#include "scl/protocol/base.h"
#include "scl/simulation/config.h"
#include "scl/simulation/context.h"

namespace scl::sim {

/**
 * @brief Manager for a simulation.
 *
 * The role of a Manager object is to describe the different parameters that
 * goes into simulation, such as how the network behaves, how to handle outputs
 * and for how many replications to run.
 */
class Manager {
 public:
  /**
   * @brief Construct a new manager.
   * @param replications the number of replications to simulate.
   */
  Manager(std::size_t replications) : m_replications(replications) {}

  /**
   * @brief Destructor.
   */
  virtual ~Manager() {}

  /**
   * @brief Get the number of replications.
   */
  std::size_t Replications() const {
    return m_replications;
  }

  /**
   * @brief Return a fresh instance of the protocol to simulate.
   *
   * Each simulation replication requires a <i>fresh</i> protocol instance to
   * run. This function takes care of returning such a protocol. The simulator
   * is assumed to take complete ownership over the returned protocol, so it is
   * important that objects returned by this function are independent of objects
   * previously returned by calling this function.
   */
  virtual std::vector<std::unique_ptr<proto::Protocol>> Protocol() = 0;

  /**
   * @brief Handle the output produced by some party.
   * @param replication the replication that the output was produced in.
   * @param party_id the ID of the party who produced the output.
   * @param output the output.
   *
   * The default implementation simply discards the output.
   */
  virtual void HandleOutput(std::size_t replication,
                            std::size_t party_id,
                            const std::any& output) {
    (void)replication;
    (void)party_id;
    (void)output;
  }

  /**
   * @brief Get the configuration for the network.
   *
   * The default is to return a SimpleNetworkConfig instance.
   */
  virtual std::shared_ptr<NetworkConfig> NetworkConfiguration() {
    return std::make_shared<SimpleNetworkConfig>();
  }

  /**
   * @brief Decide whether to terminate a party.
   * @param party_id the ID of the party.
   * @param view a view of the simulation context.
   *
   * <p>Under normal circumstances, a party is terminated when its Run function
   * returns <code>nullptr</code>. This function can be used to terminate a
   * party prematurely, e.g., after it has been running for a certain amount of
   * time.
   *
   * <p>The default implementation never terminates parties prematurely.
   */
  virtual bool Terminate(std::size_t party_id, const Context::View& view) {
    (void)party_id;
    (void)view;
    return false;
  }

 private:
  std::size_t m_replications;
};

/**
 * @brief A simple simulation manager which allows running a protocol once.
 */
class SingleReplicationManager final : public Manager {
 public:
  /**
   * @brief Construct a new SingleReplicationManager.
   * @param protocol the protocol to run
   */
  SingleReplicationManager(
      std::vector<std::unique_ptr<proto::Protocol>> protocol)
      : Manager(1), m_protocol(std::move(protocol)), m_used(false) {}

  /**
   * @brief Get the protocol to simulate.
   * @throws std::logic_error if this function is called more than once.
   */
  std::vector<std::unique_ptr<proto::Protocol>> Protocol() {
    if (m_used) {
      throw std::logic_error(
          "Protocol called twice on SingleReplicationManager");
    }
    m_used = true;
    return std::move(m_protocol);
  }

 private:
  std::vector<std::unique_ptr<proto::Protocol>> m_protocol;
  bool m_used;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_MANAGER_H

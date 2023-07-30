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

#ifndef SCL_SIMULATION_RESULT_H
#define SCL_SIMULATION_RESULT_H

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/event.h"
#include "scl/simulation/measurement.h"

namespace scl::sim {

/**
 * @brief The simulation result of a party.
 *
 * <p>Result holds measurements related to the execution of a single party from
 * a simulation. A Result holds three types of information: Measurements related
 * to execution time, measurements relating to the amount of data sent and
 * received, and the original simulation trace(s).
 *
 * <p>The main API of Result consists of the functions ExecutionTime(), which
 * provides measurements for the exeuction time of a party, and
 * TransferAmounts(), which provide measurements for the amount of data sent and
 * received by the party.
 *
 * <p>For both ExecutionTime() and TransferAmounts(), it is possible to specify
 * a "segment" when querying for measurements, by supplying a
 * <code>std::string</code> with the name of the segment. The name supplied must
 * match the name of a proto::Protocol from the protocol being simulated.
 *
 * <p>For TransferAmounts(), it is also possible to query for data sent or
 * received on a particular channel.
 */
class Result {
 public:
  /**
   * @brief Type of a segment name.
   */
  using SegmentName = std::optional<std::string>;

  /**
   * @brief Struct containing a measurement for a particular protocol segment.
   */
  struct SegmentMeasurement {
    /**
     * @brief Measurement related to execution time.
     */
    TimeMeasurement duration_m;
    /**
     * @brief Measurement relating to data sent/received.
     */
    SendRecvMeasurement send_recv_m;
    /**
     * @brief Measurements related to individual channels.
     */
    std::unordered_map<std::size_t, SendRecvMeasurement> channels_m;
  };

  /**
   * @brief Create a simulation result from a list of simulation traces.
   * @param traces the simulation traces.
   * @return a list of Results; one per party.
   *
   * <p>This function is used by Simulate() to create its return value after
   * running a simulation. The input to this function is a list of traces
   * <code>traces</code> where <code>traces[i][j]</code> is trace from i'th
   * replication of party j.
   *
   * <p>Internally, this function will collect and aggregate all traces created
   * when simulation a party, and output a Result object for each party.
   */
  static std::vector<Result> Create(
      const std::vector<std::vector<SimulationTrace>>& traces);

  /**
   * @brief Get the execution time.
   * @param name the segment. None if the total time should be returned.
   * @return a sim::TimeMeasurement.
   */
  TimeMeasurement ExecutionTime(const SegmentName& name = {}) const {
    return m_measurements.at(name).duration_m;
  }

  /**
   * @brief Get the amount of data transferred.
   * @param name the segment. None if the total amount should be returned.
   * @return a SendRecvMeasurement.
   */
  SendRecvMeasurement TransferAmounts(const SegmentName& name = {}) const {
    return m_measurements.at(name).send_recv_m;
  }

  /**
   * @brief Get the amount of data transferred on a particular channel.
   * @param id the ID of the channel.
   * @param name the segment. None if the total amount should be returned.
   * @return a SendRecvMeasurement.
   */
  SendRecvMeasurement TransferAmounts(std::size_t id,
                                      const SegmentName& name = {}) const {
    return m_measurements.at(name).channels_m.at(id);
  }

  /**
   * @brief Get the list of remote parties that this party interacted with.
   * @param name the segment. None if all interactions should be returned.
   * @return A list of party IDs.
   *
   * This returns a list of the IDs of parties that this party either sent data
   * to, or received data from.
   */
  std::vector<std::size_t> Interactions(const SegmentName& name = {}) const;

  /**
   * @brief Get the segment names of the protocol simulation.
   * @return The names of all segments in the simulated protocol.
   *
   * If any of the simulated proto::Protocol segments did not specify a name,
   * then the return value of this function will include
   * proto::Protocol::kDefaultName.
   */
  std::vector<std::string> SegmentNames() const {
    return m_segment_names;
  }

  /**
   * @brief Write a trace to a stream.
   * @param stream the stream to write the trace to.
   * @param replication the simulation replication
   * @param name the segment. None if the entire trace should be written.
   */
  void WriteTrace(std::ostream& stream,
                  std::size_t replication,
                  const SegmentName& name = {}) const;

  /**
   * @brief Write the simulation result to a stream.
   * @param stream the stream to write the result to.
   */
  void Write(std::ostream& stream) const;

  /**
   * @brief Get the simulation trace from a particular replication.
   * @param replication the replication.
   * @return the simulation trace from a replication.
   */
  SimulationTrace Trace(std::size_t replication) const {
    return m_traces[replication];
  }

  /**
   * @brief Get the measurement associated with a checkpoint.
   * @param key the string identifying the checkpoint.
   * @return the time measurement.
   */
  TimeMeasurement Checkpoint(const std::string& key) const {
    return m_checkpoints.at(key);
  }

 private:
  static Result Create(const std::vector<SimulationTrace>& traces);

  Result(
      const std::vector<SimulationTrace>& traces,
      const std::unordered_map<SegmentName, SegmentMeasurement>& measurements,
      const std::unordered_map<std::string, TimeMeasurement>& checkpoints,
      const std::vector<std::string>& segment_names)
      : m_traces(traces),
        m_measurements(measurements),
        m_checkpoints(checkpoints),
        m_segment_names(segment_names){};

  // The raw simulation trace
  std::vector<SimulationTrace> m_traces;

  // per-segment measurements
  std::unordered_map<SegmentName, SegmentMeasurement> m_measurements;

  // user made checkpoints
  std::unordered_map<std::string, TimeMeasurement> m_checkpoints;

  // segment names
  std::vector<std::string> m_segment_names;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_RESULT_H

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

#ifndef SCL_SIMULATION_EVENT_H
#define SCL_SIMULATION_EVENT_H

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "scl/util/time.h"

namespace scl::sim {

/**
 * @brief A measurement created by the simulator.
 */
class Event {
 public:
  /**
   * @brief Type describing the context in which the measurement was made.
   */
  enum class Type {
    /**
     * @brief Event indicating that a party started running.
     */
    START,

    /**
     * @brief Event indicating that a party stopped running.
     */
    STOP,

    /**
     * @brief Event indicating a network Send operation.
     */
    SEND,

    /**
     * @brief Event indicating a network Recv operation.
     */
    RECV,

    /**
     * @brief Event indicating that a party queried a channel for data.
     */
    HAS_DATA,

    /**
     * @brief Event indicating a party closed a connection.
     */
    CLOSE,

    /**
     * @brief Event indicating a party put its thread to sleep.
     */
    SLEEP,

    /**
     * @brief Event indicating that a party produces output.
     */
    OUTPUT,

    /**
     * @brief Event made at the start of a protocol segment.
     */
    SEGMENT_BEGIN,

    /**
     * @brief Event made at the end of a protocol segment.
     */
    SEGMENT_END,

    /**
     * @brief A checkpoint recorded by the protocol.
     */
    CHECKPOINT
  };

  /**
   * @brief Construct a new measurement.
   * @param type the type of the measurement
   * @param timestamp the timepoint for this measurement
   * @param offset an offset for the timestamp
   */
  Event(Type type, util::Time::Duration timestamp, util::Time::Duration offset)
      : m_type(type), m_timestamp(timestamp), m_offset(offset){};

  /**
   * @brief Construct a new measurement.
   * @param type the type of the measurement
   * @param timestamp the timepoint for this measurement.
   */
  Event(Type type, util::Time::Duration timestamp)
      : Event(type, timestamp, util::Time::Duration::zero()) {}

  virtual ~Event(){};

  /**
   * @brief Get the adjusted time of this event.
   *
   * This will return the adjusted (i.e., "real-time") time of the event. The
   * un-adjusted timestamp is <code>Time() - Offset()</code>.
   */
  util::Time::Duration Timestamp() const {
    return m_timestamp + m_offset;
  }

  /**
   * @brief Get the type of this event.
   */
  Type EventType() const {
    return m_type;
  }

  /**
   * @brief Get the offset of the timestamp of this event.
   */
  util::Time::Duration Offset() const {
    return m_offset;
  }

 private:
  Type m_type;
  util::Time::Duration m_timestamp;
  util::Time::Duration m_offset;
};

/**
 * @brief A measurement related to a channel operation.
 */
class NetworkEvent final : public Event {
 public:
  /**
   * @brief Construct a new network measurement.
   * @param type the type of the measurement
   * @param timestamp the time of the measurement
   * @param local_party the ID of the local party
   * @param remote_party the ID of the remote party
   * @param amount the amount of bytes either sent or received
   */
  NetworkEvent(Type type,
               util::Time::Duration timestamp,
               std::size_t local_party,
               std::size_t remote_party,
               std::size_t amount)
      : Event(type, timestamp),
        m_local(local_party),
        m_remote(remote_party),
        m_amount(amount) {}

  /**
   * @brief Construct a new network measurement with an offset.
   * @param type the type of the measurement
   * @param timestamp the time of the measurement
   * @param offset an offset
   * @param local_party the ID of the local party
   * @param remote_party the ID of the remote party
   * @param amount the amount of bytes either sent or received
   */
  NetworkEvent(Type type,
               util::Time::Duration timestamp,
               util::Time::Duration offset,
               std::size_t local_party,
               std::size_t remote_party,
               std::size_t amount)
      : Event(type, timestamp, offset),
        m_local(local_party),
        m_remote(remote_party),
        m_amount(amount) {}

  /**
   * @brief Get the ID of the local party in this network event.
   */
  std::size_t LocalParty() const {
    return m_local;
  }

  /**
   * @brief Get the ID of the remote party in this network event.
   */
  std::size_t RemoteParty() const {
    return m_remote;
  }

  /**
   * @brief Get the amount of data being sent/received.
   */
  std::size_t DataAmount() const {
    return m_amount;
  }

 private:
  std::size_t m_local;
  std::size_t m_remote;
  std::size_t m_amount;
};

/**
 * @brief An event taken at the start or end of <code>Protocol::Run</code>.
 */
class SegmentEvent final : public Event {
 public:
  /**
   * @brief Construct a new segment event.
   * @param type the type. Either SEGMENT_BEGIN or SEGMENT_END
   * @param timestamp the time of the event
   * @param name the name of the segment.
   */
  SegmentEvent(Type type, util::Time::Duration timestamp, std::string name)
      : Event(type, timestamp), m_name(std::move(name)){};

  /**
   * @brief Get the name of this segment.
   */
  std::string Name() const {
    return m_name;
  }

 private:
  std::string m_name;
};

/**
 * @brief An event created when a protocol calls
 * <code>env.clock.Checkpoint()</code>.
 */
class CheckpointEvent final : public Event {
 public:
  /**
   * @brief Create a new checkpoint event.
   * @param timestamp the time of the event.
   * @param message the message of the checkpoint.
   */
  CheckpointEvent(util::Time::Duration timestamp, const std::string& message)
      : Event(Event::Type::CHECKPOINT, timestamp), m_message(message) {}

  /**
   * @brief Get the checkpoint message.
   */
  std::string Message() const {
    return m_message;
  }

 private:
  std::string m_message;
};

/**
 * @brief Pretty print a measurement to a stream.
 */
std::ostream& operator<<(std::ostream& os, const Event* m);

/**
 * @brief A simulation trace is simply a vector of measurements.
 */
using SimulationTrace = std::vector<std::shared_ptr<Event>>;

}  // namespace scl::sim

#endif  // SCL_SIMULATION_EVENT_H

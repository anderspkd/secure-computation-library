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

#include "scl/simulation/channel_id.h"
#include "scl/util/time.h"

namespace scl::sim {

/**
 * @brief An event generated during simulation.
 */
class Event {
 public:
  /**
   * @brief Type of the event.
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
    CHECKPOINT,

    /**
     * @brief Event made when a party sends a net::Packet.
     */
    PACKET_SEND,

    /**
     * @brief Event made when a party receives a net::Packet.
     */
    PACKET_RECV,

    /**
     * @brief Event made when a party is stopped prematurely.
     */
    KILLED
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
 * @brief Events related to a network channel.
 */
class NetworkEvent : public Event {
 public:
  /**
   * @brief Construct a new network measurement.
   * @param type the type of the measurement
   * @param timestamp the time of the measurement
   * @param id the ID of the channel
   */
  NetworkEvent(Type type, util::Time::Duration timestamp, ChannelId id)
      : Event(type, timestamp), m_id(id) {}

  /**
   * @brief Construct a new network measurement with an offset.
   * @param type the type of the measurement
   * @param timestamp the time of the measurement
   * @param offset an offset
   * @param id the ID of the channel
   */
  NetworkEvent(Type type,
               util::Time::Duration timestamp,
               util::Time::Duration offset,
               ChannelId id)
      : Event(type, timestamp, offset), m_id(id) {}

  /**
   * @brief Get the ID of the local party in this network event.
   */
  std::size_t LocalParty() const {
    return m_id.local;
  }

  /**
   * @brief Get the ID of the remote party in this network event.
   */
  std::size_t RemoteParty() const {
    return m_id.remote;
  }

 private:
  ChannelId m_id;
};

/**
 * @brief Events related to data transfers on the network.
 */
class NetworkDataEvent : public NetworkEvent {
 public:
  /**
   * @brief Create a new network data event.
   * @param type the type of the event.
   * @param timestamp when the event took place.
   * @param id the ID of the channel.
   * @param amount the amount of data sent or received.
   */
  NetworkDataEvent(Type type,
                   util::Time::Duration timestamp,
                   ChannelId id,
                   std::size_t amount)
      : NetworkEvent(type, timestamp, id), m_amount(amount) {}

  /**
   * @brief Create a new network data event.
   * @param type the type of the event.
   * @param timestamp when the event took place.
   * @param offset an offset to \p timestamp.
   * @param id the ID of the channel.
   * @param amount the amount of data sent or received.
   */
  NetworkDataEvent(Type type,
                   util::Time::Duration timestamp,
                   util::Time::Duration offset,
                   ChannelId id,
                   std::size_t amount)
      : NetworkEvent(type, timestamp, offset, id), m_amount(amount) {}

  /**
   * @brief Get the amount of data sent or received.
   */
  std::size_t DataAmount() const {
    return m_amount;
  }

 private:
  std::size_t m_amount;
};

/**
 * @brief An event created when a party calls the packet recv function on a
 * channel.
 */
class PacketRecvEvent final : public NetworkDataEvent {
 public:
  /**
   * @brief Create a new network data event.
   * @param timestamp when the event took place.
   * @param offset an offset to \p timestamp.
   * @param id the ID of the channel.
   * @param amount the amount of data sent or received.
   * @param blocking whether the Recv call was blocking.
   */
  PacketRecvEvent(util::Time::Duration timestamp,
                  util::Time::Duration offset,
                  ChannelId id,
                  std::size_t amount,
                  bool blocking)
      : NetworkDataEvent(Type::PACKET_RECV, timestamp, offset, id, amount),
        m_blocking(blocking) {}

  /**
   * @brief True if the call was blocking and false otherwise.
   */
  bool Blocking() const {
    return m_blocking;
  }

 private:
  bool m_blocking;
};

/**
 * @brief Event created when a party calls HasData on a channel.
 */
class HasDataEvent final : public NetworkEvent {
 public:
  /**
   * @brief Construct a new HasDataEvent.
   * @param timestamp the time the event happened.
   * @param id the ID of the channel.
   * @param had_data whether data was available.
   */
  HasDataEvent(util::Time::Duration timestamp, ChannelId id, bool had_data)
      : NetworkEvent(Type::HAS_DATA, timestamp, id), m_had_data(had_data) {}

  /**
   * @brief Whether the call that generated this event had data.
   */
  bool HadData() const {
    return m_had_data;
  }

 private:
  bool m_had_data;
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
   * @param id the id of the checkpoint.
   */
  CheckpointEvent(util::Time::Duration timestamp, const std::string& id)
      : Event(Event::Type::CHECKPOINT, timestamp), m_id(id) {}

  /**
   * @brief Get the checkpoint id.
   */
  std::string Id() const {
    return m_id;
  }

 private:
  std::string m_id;
};

/**
 * @brief Pretty print an event type.
 */
std::ostream& operator<<(std::ostream& os, Event::Type type);

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

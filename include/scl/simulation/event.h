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
 * @brief Event types.
 */
enum class EventType {
  /**
   * @brief Event generated when a party starts running.
   */
  START,

  /**
   * @brief Event generated when a party stops running.
   */
  STOP,

  /**
   * @brief Event generated when a party is forcibly stopped.
   */
  KILLED,

  /**
   * @brief Event generated when a party was cancelled by the manager.
   */
  CANCELLED,

  /**
   * @brief Event generated when a channel is closed.
   */
  CLOSE,

  /**
   * @brief Event generated when data is sent on a channel.
   */
  SEND,

  /**
   * @brief Event generated when data is received on a channel.
   */
  RECV,

  /**
   * @brief Event generated when a channel is queried for the presence of data.
   */
  HAS_DATA,

  /**
   * @brief Event generated when a party sleeps.
   */
  SLEEP,

  /**
   * @brief Event generated when a party produces output.
   */
  OUTPUT,

  /**
   * @brief Event generated at the start of a protocol.
   */
  PROTOCOL_BEGIN,

  /**
   * @brief Event generated at the end of a protocol.
   */
  PROTOCOL_END,

};

/**
 * @brief An event in a simulation.
 */
struct Event {
  /**
   * @brief Create an event indicating the party started running.
   */
  static std::shared_ptr<Event> start();

  /**
   * @brief Create an event indicating the party stopped running.
   * @param timestamp the time the party stopped running at.
   */
  static std::shared_ptr<Event> stop(util::Time::Duration timestamp);

  /**
   * @brief Create an event indicating the party was killed by an exception.
   * @param timestamp the time the party was stopped.
   * @param reason a message describing the reason for the kill.
   */
  static std::shared_ptr<Event> killed(util::Time::Duration timestamp,
                                       const std::string& reason);

  /**
   * @brief Create an event indicating the party was stopped.
   * @param timestamp the time the party was stopped.
   */
  static std::shared_ptr<Event> cancelled(util::Time::Duration timestamp);

  /**
   * @brief Create an event indicating that a channel was closed.
   * @param timestamp the time the channel was closed.
   * @param channel_id the ID of the channel.
   */
  static std::shared_ptr<Event> closeChannel(util::Time::Duration timestamp,
                                             ChannelId channel_id);

  /**
   * @brief Create an event indicating that some data was sent on a channel.
   * @param timestamp the time the data was sent.
   * @param channel_id the ID of the channel.
   * @param amount the amount of bytes sent.
   */
  static std::shared_ptr<Event> sendData(util::Time::Duration timestamp,
                                         ChannelId channel_id,
                                         std::size_t amount);

  /**
   * @brief Create an event indicating that some data was received on a channel.
   * @param timestamp the time the data was received.
   * @param channel_id the ID of the channel.
   * @param amount the amount of bytes received.
   */
  static std::shared_ptr<Event> recvData(util::Time::Duration timestamp,
                                         ChannelId channel_id,
                                         std::size_t amount);

  /**
   * @brief Create an event indicating that a channel was queried for the
   * presence of data.
   * @param timestamp the time of the query.
   * @param channel_id the ID of the channel.
   */
  static std::shared_ptr<Event> hasData(util::Time::Duration timestamp,
                                        ChannelId channel_id);

  /**
   * @brief Create an event indicating that the party slept.
   * @param timestamp the time the party went to sleep.
   * @param sleep_duration the duration of the sleep.
   */
  static std::shared_ptr<Event> sleep(util::Time::Duration timestamp,
                                      util::Time::Duration sleep_duration);

  /**
   * @brief Create an event indicating that the party produced an output.
   */
  static std::shared_ptr<Event> output(util::Time::Duration timestamp);

  /**
   * @brief Create an event indicating that a protocol began.
   * @param timestamp the starting time of the protocol.
   * @param protocol_name the name of the protocol.
   */
  static std::shared_ptr<Event> protocolBegin(util::Time::Duration timestamp,
                                              const std::string& protocol_name);

  /**
   * @brief Create an event indicating that a protocol ended.
   * @param timestamp the finishing time of the protocol.
   * @param protocol_name the name of the protocol.
   */
  static std::shared_ptr<Event> protocolEnd(util::Time::Duration timestamp,
                                            const std::string& protocol_name);

  /**
   * @brief Constructor.
   */
  Event(EventType type, util::Time::Duration timestamp)
      : type(type), timestamp(timestamp) {}

  virtual ~Event() {}

  /**
   * @brief The event type.
   */
  EventType type;

  /**
   * @brief The event timestamp.
   */
  util::Time::Duration timestamp;
};

/**
 * @brief An event relating to a channel.
 */
struct ChannelEvent : public Event {
  /**
   * @brief Constructor
   */
  ChannelEvent(EventType type,
               util::Time::Duration timestamp,
               ChannelId channel_id)
      : Event(type, timestamp), channel_id(channel_id) {}

  ~ChannelEvent() {}
  /**
   * @brief The ID of the channel this event was created for.
   */
  ChannelId channel_id;
};

/**
 * @brief An event relating to a channel send or receive action.
 */
struct ChannelDataEvent final : public ChannelEvent {
  /**
   * @brief Constructor.
   */
  ChannelDataEvent(EventType type,
                   util::Time::Duration timestamp,
                   ChannelId channel_id,
                   std::size_t amount)
      : ChannelEvent(type, timestamp, channel_id), amount(amount) {}

  /**
   * @brief The amount of data in this event.
   */
  std::size_t amount;
};

/**
 * @brief An event relating to a sleep.
 */
struct SleepEvent final : public Event {
  /**
   * @brief Constructor.
   */
  SleepEvent(EventType type,
             util::Time::Duration timestamp,
             util::Time::Duration sleep_duration)
      : Event(type, timestamp + sleep_duration),
        sleep_duration(sleep_duration) {}
  /**
   * @brief The sleep duration.
   */
  util::Time::Duration sleep_duration;
};

/**
 * @brief A protocol event.
 */
struct ProtocolEvent final : public Event {
  /**
   * @brief Constructor.
   */
  ProtocolEvent(EventType type,
                util::Time::Duration timestamp,
                const std::string& protocol_name)
      : Event(type, timestamp), protocol_name(protocol_name) {}
  /**
   * @brief The name of the protocol.
   */
  std::string protocol_name;
};

/**
 * @brief A kill event.
 */
struct KillEvent final : public Event {
  /**
   * @brief Constructor.
   */
  KillEvent(util::Time::Duration timestamp, const std::string& reason)
      : Event(EventType::KILLED, timestamp), reason(reason) {}

  /**
   * @brief The message giving a reason for the kill.
   */
  std::string reason;
};

/**
 * @brief Pretty print an event type.
 */
std::ostream& operator<<(std::ostream& stream, EventType type);

/**
 * @brief Pretty print an event.
 */
std::ostream& operator<<(std::ostream& stream, const Event* event);

/**
 * @brief Pretty print an event.
 */
inline std::ostream& operator<<(std::ostream& stream,
                                std::shared_ptr<Event> event) {
  return stream << event.get();
}

/**
 * @brief The execution trace of a simulation is a list of the events it
 * generated.
 */
using SimulationTrace = std::vector<std::shared_ptr<Event>>;

/**
 * @brief Write a trace to an output stream.
 * @param stream the stream.
 * @param trace the trace.
 */
void writeTrace(std::ostream& stream, const SimulationTrace& trace);

}  // namespace scl::sim

#endif  // SCL_SIMULATION_EVENT_H

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
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "scl/simulation/channel_id.h"
#include "scl/time.h"

namespace scl {

/**
 * @brief Event types.
 * @ingroup eval-sim
 */
enum class EventType {
  /**
   * @brief Internal event type.
   */
  TRANSIENT,

  /**
   * @brief Events emitted when a Protocol is begins executing.
   */
  BEGIN,

  /**
   * @brief Events emitted when a Protocol ends.
   */
  END,

  /**
   * @brief Event emitted when a simulation starts.
   */
  START,

  /**
   * @brief Event emitted when a simulation ends.
   */
  STOP,

  /**
   * @brief Event emitted if the simulation was stopped due to an exception.
   */
  KILLED,

  /**
   * @brief Event emitted if the simulation was stopped by a user defined hook.
   */
  CANCELLED,

  /**
   * @brief Event emitted when a Channel is closed.
   */
  CHANNEL_CLOSE,

  /**
   * @brief Event emitted when Packet is sent on a Channel.
   */
  CHANNEL_SEND,

  /**
   * @brief Event emitted when a Packet is received on a Channel.
   */
  CHANNEL_RECV,

  /**
   * @brief Event emitted if a timeout occured when receiving a Packet.
   */
  CHANNEL_RECV_TIMEOUT,

  /**
   * @brief Event emitted when a Channel is poll'ed for data.
   */
  CHANNEL_POLL,

  /**
   * @brief Event emitted when a protocol sleeps.
   */
  SLEEP
};

/**
 * @brief Base class for all events.
 * @ingroup eval-sim
 *
 * All events in SCL's simulator inheret from Event. The most basic Event is
 * effectively just a timestamp telling us when the event was generated.
 */
class Event {
 public:
  /**
   * @brief Constructor.
   */
  Event(Time::Duration timestamp) : m_timestamp(timestamp) {}

  virtual ~Event() {}

  /**
   * @brief Returns the timestamp of this event.
   */
  virtual Time::Duration time() const {
    return m_timestamp;
  }

  /**
   * @brief Write a representation of this event to a stream.
   */
  virtual void write(std::ostream& stream) = 0;

  /**
   * @brief Get a type descriptor of the event.
   */
  virtual EventType type() const = 0;

 private:
  Time::Duration m_timestamp;
};

/**
 * @brief Event issued when a party starts executing a Protocol.
 * @ingroup eval-sim
 * @see EndEvent
 */
class BeginEvent final : public Event {
 public:
  /**
   * @brief Construct a new BeginEvent.
   */
  BeginEvent(Time::Duration timestamp, const std::string& name)
      : Event(timestamp), m_name(name) {}

  void write(std::ostream& stream) override;

  EventType type() const override {
    return EventType::BEGIN;
  }

  /**
   * @brief The name of protocol.
   * @see Protocol::name
   */
  std::string_view name() const {
    return m_name;
  }

 private:
  std::string m_name;
};

/**
 * @brief Event issued when a party finishes executing a Protocol.
 * @ingroup eval-sim
 * @see BeginEvent
 */
class EndEvent final : public Event {
 public:
  /**
   * @brief Construct a new EndEvent.
   */
  EndEvent(Time::Duration timestamp, const std::string& name)
      : Event(timestamp), m_name(name) {}

  void write(std::ostream& stream) override;

  EventType type() const override {
    return EventType::END;
  }

  /**
   * @brief The name of protocol.
   * @see Protocol::name
   */
  std::string_view name() const {
    return m_name;
  }

 private:
  std::string m_name;
};

/**
 * @brief Event issued when a party starts running.
 * @ingroup eval-sim
 * @see StopEvent
 */
class StartEvent final : public Event {
 public:
  /**
   * @brief Construct a new start event.
   */
  StartEvent() : Event(Time::Duration::zero()) {}
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::START;
  }
};

/**
 * @brief Event issued when a party stops running.
 * @ingroup eval-sim
 * @see StartEvent
 */
class StopEvent final : public Event {
 public:
  using Event::Event;
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::STOP;
  }
};

/**
 * @brief Event issued if a party throws an uncaught exception.
 * @ingroup eval-sim
 */
class KilledEvent final : public Event {
 public:
  /**
   * @brief Construct a new KilledEvent.
   */
  KilledEvent(Time::Duration timestamp, const std::string& reason)
      : Event(timestamp), m_reason(reason) {}

  /**
   * @brief The exception's what message.
   */
  std::string reason() const {
    return m_reason;
  }
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::KILLED;
  }

 private:
  std::string m_reason;
};

/**
 * @brief Event issued if a party gets killed by a user specified hook.
 */
class CancelledEvent final : public Event {
 public:
  using Event::Event;
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::CANCELLED;
  }
};

/**
 * @brief Base class for events related to channel actions.
 */
class ChannelEvent : public Event {
 public:
  /**
   * @brief Construct a new ChannelEvent.
   */
  ChannelEvent(Time::Duration timestamp, ChannelId id)
      : Event(timestamp), m_id(id) {}

  /**
   * @brief The identifier of the channel this event pertains to.
   */
  ChannelId id() const {
    return m_id;
  }

 private:
  ChannelId m_id;
};

/**
 * @brief Event issued when a party closes a channel.
 */
class CloseEvent final : public ChannelEvent {
 public:
  using ChannelEvent::ChannelEvent;
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::CHANNEL_CLOSE;
  }
};

/**
 * @brief Base class for events related to data transfers on a channel.
 */
class ChannelDataEvent : public ChannelEvent {
 public:
  /**
   * @brief Construct a new ChannelDataEvent.
   */
  ChannelDataEvent(Time::Duration timestamp, ChannelId id, std::size_t amount)
      : ChannelEvent(timestamp, id), m_amount(amount) {}

  /**
   * @brief The amount of data sent or received.
   */
  std::size_t amount() const {
    return m_amount;
  }

 private:
  std::size_t m_amount;
};

/**
 * @brief Event issued when a party sends data on a channel.
 */
class SendEvent final : public ChannelDataEvent {
 public:
  using ChannelDataEvent::ChannelDataEvent;
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::CHANNEL_SEND;
  }
};

/**
 * @brief Event issued when a party finishes receiving data.
 */
class RecvEvent final : public ChannelDataEvent {
 public:
  using ChannelDataEvent::ChannelDataEvent;
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::CHANNEL_RECV;
  }
};

/**
 * @brief Event issued when a party times out during a recv call.
 */
class RecvTimeoutEvent final : public ChannelEvent {
 public:
  using ChannelEvent::ChannelEvent;
  void write(std::ostream& stream) override;
  EventType type() const override {
    return EventType::CHANNEL_RECV_TIMEOUT;
  }
};

/**
 * @brief Event issued by a party when it issues call to Channel::poll.
 */
class PollEvent final : public ChannelEvent {
 public:
  PollEvent(Time::Duration timestamp, ChannelId id, bool result)
      : ChannelEvent(timestamp, id), m_result(result) {}
  void write(std::ostream& stream) override;

  EventType type() const override {
    return EventType::CHANNEL_POLL;
  }

  /**
   * @brief The result of the Channel::poll call.
   */
  bool result() const {
    return m_result;
  }

 private:
  bool m_result;
};

/**
 * @brief Event issued when a party sleeps.
 */
class SleepEvent final : public Event {
 public:
  SleepEvent(Time::Duration timestamp, Time::Duration duration)
      : Event(timestamp), m_duration(duration) {};

  void write(std::ostream& stream) override;

  EventType type() const override {
    return EventType::SLEEP;
  }

  /**
   * @brief The amount of time a party slept.
   */
  Time::Duration duration() const {
    return m_duration;
  }

 private:
  Time::Duration m_duration;
};

namespace details {

/**
 * @brief Tracks events added by a party during simulation.
 *
 * EventList is mostly just a wrapper around an STL vector of Event
 * pointers. Minor book keeping is done to ensure that all events of type
 * EventType::TRANSIENT only appear of the head of the list.
 */
class EventList final {
 public:
  EventList();

  /**
   * @brief Add a new event by in-place construction.
   */
  template <typename EVENT, typename... ARGS>
    requires(std::is_base_of_v<Event, EVENT>)
  void add(ARGS... args) {
    // if the last event is transient, replace it instead.
    if (!m_events.empty() && m_events.back()->type() == EventType::TRANSIENT) {
      m_events.back() = std::make_unique<EVENT>(std::forward<ARGS>(args)...);
    } else {
      m_events.emplace_back(
          std::make_unique<EVENT>(std::forward<ARGS>(args)...));
    }
  }

  auto begin() {
    return m_events.begin();
  }

  auto cbegin() const {
    return m_events.cbegin();
  }

  auto end() {
    return m_events.end();
  }

  auto cend() const {
    return m_events.cend();
  }

  auto size() const {
    return m_events.size();
  }

  bool empty() const {
    return m_events.empty();
  }

  const Event* latest() const {
    return m_events.back().get();
  }

  Event* latest() {
    return m_events.back().get();
  }

 private:
  std::vector<std::unique_ptr<Event>> m_events;
};

}  // namespace details
}  // namespace scl

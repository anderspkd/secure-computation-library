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

#include "scl/simulation/event.h"

#include <chrono>

#include "scl/simulation/channel_id.h"
#include "scl/time.h"

std::shared_ptr<scl::Event> scl::Event::start() {
  return std::make_shared<Event>(EventType::START, Time::Duration::zero());
}

std::shared_ptr<scl::Event> scl::Event::stop(Time::Duration timestamp) {
  return std::make_shared<Event>(EventType::STOP, timestamp);
}

std::shared_ptr<scl::Event> scl::Event::killed(Time::Duration timestamp,
                                               const std::string& reason) {
  return std::make_shared<KillEvent>(timestamp, reason);
}

std::shared_ptr<scl::Event> scl::Event::cancelled(Time::Duration timestamp) {
  return std::make_shared<Event>(EventType::CANCELLED, timestamp);
}

std::shared_ptr<scl::Event> scl::Event::closeChannel(
    Time::Duration timestamp,
    scl::ChannelId channel_id) {
  return std::make_shared<ChannelEvent>(EventType::CLOSE,
                                        timestamp,
                                        channel_id);
}

std::shared_ptr<scl::Event> scl::Event::sendData(Time::Duration timestamp,
                                                 scl::ChannelId channel_id,
                                                 std::size_t amount) {
  return std::make_shared<ChannelDataEvent>(EventType::SEND,
                                            timestamp,
                                            channel_id,
                                            amount);
}

std::shared_ptr<scl::Event> scl::Event::recvData(Time::Duration timestamp,
                                                 scl::ChannelId channel_id,
                                                 std::size_t amount) {
  return std::make_shared<ChannelDataEvent>(EventType::RECV,
                                            timestamp,
                                            channel_id,
                                            amount);
}

std::shared_ptr<scl::Event> scl::Event::hasData(Time::Duration timestamp,
                                                scl::ChannelId channel_id) {
  return std::make_shared<ChannelEvent>(EventType::HAS_DATA,
                                        timestamp,
                                        channel_id);
}

std::shared_ptr<scl::Event> scl::Event::sleep(Time::Duration timestamp,
                                              Time::Duration sleep_duration) {
  return std::make_shared<SleepEvent>(EventType::SLEEP,
                                      timestamp,
                                      sleep_duration);
}

std::shared_ptr<scl::Event> scl::Event::output(Time::Duration timestamp) {
  return std::make_shared<Event>(EventType::OUTPUT, timestamp);
}

std::shared_ptr<scl::Event> scl::Event::protocolBegin(
    Time::Duration timestamp,
    const std::string& protocol_name) {
  return std::make_shared<ProtocolEvent>(EventType::PROTOCOL_BEGIN,
                                         timestamp,
                                         protocol_name);
}

std::shared_ptr<scl::Event> scl::Event::protocolEnd(
    Time::Duration timestamp,
    const std::string& protocol_name) {
  return std::make_shared<ProtocolEvent>(EventType::PROTOCOL_END,
                                         timestamp,
                                         protocol_name);
}

namespace {

std::string eventTypeToString(scl::EventType type) {
  switch (type) {
    case scl::EventType::START:
      return "START";
      break;
    case scl::EventType::STOP:
      return "STOP";
      break;
    case scl::EventType::SEND:
      return "SEND";
      break;
    case scl::EventType::RECV:
      return "RECV";
      break;
    case scl::EventType::HAS_DATA:
      return "HAS_DATA";
      break;
    case scl::EventType::OUTPUT:
      return "OUTPUT";
      break;
    case scl::EventType::SLEEP:
      return "SLEEP";
      break;
    case scl::EventType::PROTOCOL_BEGIN:
      return "PROTOCOL_BEGIN";
      break;
    case scl::EventType::PROTOCOL_END:
      return "PROTOCOL_END";
      break;
    case scl::EventType::KILLED:
      return "KILLED";
      break;
    case scl::EventType::CANCELLED:
      return "CANCELLED";
      break;
      // case scl::EventType::CLOSE:
    default:
      return "CLOSE";
  }
}

void writeObj(std::ostream& stream, const std::string& string) {
  stream << "\"" << string << "\"";
}

void writeKey(std::ostream& stream, const std::string& name) {
  writeObj(stream, name);
  stream << ":";
}

void writeObj(std::ostream& stream, const std::size_t& val) {
  stream << val;
}

void writeObj(std::ostream& stream, const long double& val) {
  stream << val;
}

void writeObj(std::ostream& stream, const scl::Time::Duration& d) {
  auto t = std::chrono::duration<long double, std::milli>(d).count();
  writeObj(stream, t);
}

void writeObj(std::ostream& stream, const scl::ChannelId& id) {
  stream << "{";

  writeKey(stream, "local");
  writeObj(stream, id.local);

  stream << ",";

  writeKey(stream, "remote");
  writeObj(stream, id.remote);

  stream << "}";
}

void writeEvent(std::ostream& stream, const scl::ChannelEvent* event) {
  stream << "{";

  writeKey(stream, "channel_id");
  writeObj(stream, event->channel_id);

  stream << "}";
}

void writeEvent(std::ostream& stream, const scl::ChannelDataEvent* event) {
  stream << "{";

  writeKey(stream, "channel_id");
  writeObj(stream, event->channel_id);

  stream << ",";

  writeKey(stream, "amount");
  writeObj(stream, event->amount);

  stream << "}";
}

void writeEvent(std::ostream& stream, const scl::SleepEvent* event) {
  stream << "{";

  writeKey(stream, "duration");
  writeObj(stream, event->sleep_duration);

  stream << "}";
}

void writeEvent(std::ostream& stream, const scl::ProtocolEvent* event) {
  stream << "{";

  writeKey(stream, "name");
  writeObj(stream, event->protocol_name);

  stream << "}";
}

void writeEvent(std::ostream& stream, const scl::KillEvent* event) {
  stream << "{";

  writeKey(stream, "reason");
  writeObj(stream, event->reason);

  stream << "}";
}

}  // namespace

std::ostream& scl::operator<<(std::ostream& stream,
                              const scl::EventType event_type) {
  return stream << eventTypeToString(event_type);
}

std::ostream& scl::operator<<(std::ostream& stream, const scl::Event* event) {
  stream << "{";

  writeKey(stream, "timestamp");
  writeObj(stream, timeToMillis(event->timestamp));

  stream << ",";

  writeKey(stream, "type");
  writeObj(stream, eventTypeToString(event->type));

  stream << ",";

  writeKey(stream, "metadata");

  switch (event->type) {
    case EventType::CLOSE:
    case EventType::HAS_DATA:
      writeEvent(stream, dynamic_cast<const ChannelEvent*>(event));
      break;

    case EventType::SEND:
    case EventType::RECV:
      writeEvent(stream, dynamic_cast<const ChannelDataEvent*>(event));
      break;

    case EventType::SLEEP:
      writeEvent(stream, dynamic_cast<const SleepEvent*>(event));
      break;

    case EventType::PROTOCOL_BEGIN:
    case EventType::PROTOCOL_END:
      writeEvent(stream, dynamic_cast<const ProtocolEvent*>(event));
      break;

    case EventType::KILLED:
      writeEvent(stream, dynamic_cast<const KillEvent*>(event));
      break;

    default:
      stream << "{}";
      break;
  }

  stream << "}";

  return stream;
}

void scl::writeTrace(std::ostream& stream, const scl::SimulationTrace& trace) {
  stream << "[";

  if (!trace.empty()) {
    for (std::size_t i = 0; i < trace.size() - 1; i++) {
      stream << trace[i] << ",";
    }
    stream << trace[trace.size() - 1];
  }

  stream << "]";
}

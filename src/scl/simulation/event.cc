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

#include "scl/simulation/event.h"

#include <ostream>
#include <string_view>

#include "scl/simulation/channel_id.h"
#include "scl/time.h"

using namespace scl;

namespace {

void writeNumeric(std::ostream& stream, std::string_view key, std::size_t n) {
  stream << "\"" << key << "\":" << n;
}

void writeNumeric(std::ostream& stream, std::string_view key, long double n) {
  stream << "\"" << key << "\":" << n;
}

void writeString(std::ostream& stream,
                 std::string_view key,
                 std::string_view val) {
  stream << "\"" << key << "\":" << "\"" << val << "\"";
}

void writeTimestamp(std::ostream& stream, Time::Duration t) {
  // "t":<timestamp in milis>
  writeNumeric(stream, "t", timeToMillis(t));
}

void writeChannelId(std::ostream& stream, ChannelId id) {
  stream << "\"id\":" << "[" << id.local << "," << id.remote << "]";
}

}  // namespace

#define JSON_OBJ_START stream << "{"
#define JSON_OBJ_END stream << "}"
#define JSON_COMMA stream << ","

void BeginEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "BEGIN");
  JSON_COMMA;
  writeString(stream, "name", name());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void EndEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "END");
  JSON_COMMA;
  writeString(stream, "name", name());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void StartEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "START");
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void StopEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "STOP");
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void KilledEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "KILLED");
  JSON_COMMA;
  writeString(stream, "reason", reason());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void CancelledEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CANCELLED");
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void CloseEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CHANNEL_CLOSE");
  JSON_COMMA;
  writeChannelId(stream, id());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void SendEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CHANNEL_SEND");
  JSON_COMMA;
  writeChannelId(stream, id());
  JSON_COMMA;
  writeNumeric(stream, "amount", amount());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void RecvEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CHANNEL_RECV");
  JSON_COMMA;
  writeChannelId(stream, id());
  JSON_COMMA;
  writeNumeric(stream, "amount", amount());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void RecvTimeoutEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CHANNEL_RECV_TIMEOUT");
  JSON_COMMA;
  writeChannelId(stream, id());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void PollEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CHANNEL_POLL");
  JSON_COMMA;
  writeChannelId(stream, id());
  JSON_COMMA;
  writeString(stream, "result", result() ? "true" : "false");
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void SleepEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "SLEEP");
  JSON_COMMA;
  writeNumeric(stream, "duration", timeToMillis(duration()));
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

#undef JSON_OBJ_START
#undef JSON_OBJ_END
#undef JSON_COMMA

namespace {

// This "event" serves as a backstop in order to ensure that an EventList always
// contains at least one event, and that this event is meaningful (hence the
// initialization with a timestamp of 0)
struct InitialEvent final : public Event {
  InitialEvent() : Event(Time::Duration::zero()) {}

  void write(std::ostream&) override {}

  // mark this event as TRANSIENT so that it gets removed once real events
  // arrive.
  EventType type() const override {
    return EventType::TRANSIENT;
  }
};

}  // namespace

EventList::EventList() {
  add<InitialEvent>();
}

#include "scl/simulation/event.h"

#include <ostream>
#include <string_view>

#include "scl/simulation/channel_id.h"
#include "scl/time.h"

namespace {

void writeNumeric(std::ostream& stream, std::string_view key, std::size_t n) {
  stream << "\"" << key << "\":" << n;
}

void writeString(std::ostream& stream,
                 std::string_view key,
                 std::string_view val) {
  stream << "\"" << key << "\":" << "\"" << val << "\"";
}

void writeTimestamp(std::ostream& stream, scl::Time::Duration t) {
  // "t":<timestamp in milis>
  writeNumeric(stream, "t", scl::timeToMillis(t));
}

void writeChannelId(std::ostream& stream, scl::ChannelId id) {
  stream << "\"id\":" << "[" << id.local << "," << id.remote << "]";
}

}  // namespace

#define JSON_OBJ_START stream << "{"
#define JSON_OBJ_END stream << "}"
#define JSON_COMMA stream << ","

void scl::BeginEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "BEGIN");
  JSON_COMMA;
  writeString(stream, "name", name());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::EndEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "END");
  JSON_COMMA;
  writeString(stream, "name", name());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::StartEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "START");
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::StopEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "STOP");
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::KilledEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "KILLED");
  JSON_COMMA;
  writeString(stream, "reason", reason());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::CancelledEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CANCELLED");
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::CloseEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CHANNEL_CLOSE");
  JSON_COMMA;
  writeChannelId(stream, id());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::SendEvent::write(std::ostream& stream) {
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

void scl::RecvEvent::write(std::ostream& stream) {
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

void scl::RecvTimeoutEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "CHANNEL_RECV_TIMEOUT");
  JSON_COMMA;
  writeChannelId(stream, id());
  JSON_COMMA;
  writeTimestamp(stream, time());
  JSON_OBJ_END;
}

void scl::PollEvent::write(std::ostream& stream) {
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

void scl::SleepEvent::write(std::ostream& stream) {
  JSON_OBJ_START;
  writeString(stream, "type", "SLEEP");
  JSON_COMMA;
  writeNumeric(stream, "duration", scl::timeToMillis(duration()));
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
struct InitialEvent final : public scl::Event {
  InitialEvent() : scl::Event(scl::Time::Duration::zero()) {}

  void write(std::ostream&) override {}

  // mark this event as TRANSIENT so that it gets removed once real events
  // arrive.
  scl::EventType type() const override {
    return scl::EventType::TRANSIENT;
  }
};

}  // namespace

scl::EventList::EventList() {
  add<InitialEvent>();
}

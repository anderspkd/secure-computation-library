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

#include "scl/simulation/event.h"

#include <chrono>
#include <ios>

using namespace scl;

namespace {

auto EventTypeToString(sim::Event::Type type) {
  if (type == sim::Event::Type::START) {
    return "START";
  }

  if (type == sim::Event::Type::STOP) {
    return "STOP";
  }

  if (type == sim::Event::Type::SEND) {
    return "SEND";
  }

  if (type == sim::Event::Type::RECV) {
    return "RECV";
  }

  if (type == sim::Event::Type::HAS_DATA) {
    return "HAS_DATA";
  }

  if (type == sim::Event::Type::OUTPUT) {
    return "OUTPUT";
  }

  if (type == sim::Event::Type::SLEEP) {
    return "SLEEP";
  }

  if (type == sim::Event::Type::SEGMENT_BEGIN) {
    return "SEGMENT_BEGIN";
  }

  if (type == sim::Event::Type::SEGMENT_END) {
    return "SEGMENT_END";
  }

  if (type == sim::Event::Type::CHECKPOINT) {
    return "CHECKPOINT";
  }

  if (type == sim::Event::Type::PACKET_SEND) {
    return "PACKET_SEND";
  }

  if (type == sim::Event::Type::PACKET_RECV) {
    return "PACKET_RECV";
  }

  if (type == sim::Event::Type::KILLED) {
    return "KILLED";
  }

  // if (type == Measurement::Type::CLOSE)
  return "CLOSE";
}

void WriteClose(std::ostream& os, const sim::NetworkEvent* m) {
  os << " [Local=" << m->LocalParty() << ", Remote=" << m->RemoteParty() << "]";
}

void WriteSend(std::ostream& os, const sim::NetworkDataEvent* m) {
  os << " ["
     << "Sender=" << m->LocalParty() << ", Receiver=" << m->RemoteParty()
     << ", Amount=" << m->DataAmount() << "]";
}

void WriteRecv(std::ostream& os, const sim::NetworkDataEvent* m) {
  os << " ["
     << "Receiver=" << m->LocalParty() << ", Sender=" << m->RemoteParty()
     << ", Amount=" << m->DataAmount() << "]";
}

void WritePacketRecv(std::ostream& os, const sim::PacketRecvEvent* m) {
  os << " ["
     << "Receiver=" << m->LocalParty() << ", Sender=" << m->RemoteParty()
     << ", Amount=" << m->DataAmount() << ", Blocking=" << std::boolalpha
     << m->Blocking() << "]";
}

void WriteSegment(std::ostream& os, const sim::SegmentEvent* m) {
  const auto name = m->Name();
  if (name.empty()) {
    os << " [Unnamed segment]";
  } else {
    os << " [Name=" << name << "]";
  }
}

void WriteHasData(std::ostream& os, const sim::HasDataEvent* m) {
  os << " [Local=" << m->LocalParty() << ", Remote=" << m->RemoteParty()
     << ", DataAvailable=" << std::boolalpha << m->HadData() << "]";
}

void WriteCheckpoint(std::ostream& os, const sim::CheckpointEvent* m) {
  os << " [" << m->Id() << "]";
}

}  // namespace

std::ostream& sim::operator<<(std::ostream& os, Event::Type type) {
  return os << EventTypeToString(type);
}

std::ostream& sim::operator<<(std::ostream& os, const sim::Event* m) {
  using namespace std::chrono;
  const auto t = m->EventType();
  os << t << " at ";
  os << duration<double, std::milli>(m->Timestamp()).count();
  os << " ms";
  if (m->Offset() > util::Time::Duration::zero()) {
    os << " [Offset=";
    os << duration<double, std::milli>(m->Offset()).count();
    os << " ms]";
  }

  if (t == sim::Event::Type::SEGMENT_BEGIN ||
      t == sim::Event::Type::SEGMENT_END) {
    WriteSegment(os, dynamic_cast<const SegmentEvent*>(m));
  }

  if (t == sim::Event::Type::CLOSE) {
    WriteClose(os, dynamic_cast<const NetworkEvent*>(m));
  }

  if (t == sim::Event::Type::SEND || t == sim::Event::Type::PACKET_SEND) {
    WriteSend(os, dynamic_cast<const NetworkDataEvent*>(m));
  }

  if (t == sim::Event::Type::RECV) {
    WriteRecv(os, dynamic_cast<const NetworkDataEvent*>(m));
  }

  if (t == sim::Event::Type::PACKET_RECV) {
    WritePacketRecv(os, dynamic_cast<const PacketRecvEvent*>(m));
  }

  if (t == sim::Event::Type::HAS_DATA) {
    WriteHasData(os, dynamic_cast<const HasDataEvent*>(m));
  }

  if (t == sim::Event::Type::CHECKPOINT) {
    WriteCheckpoint(os, dynamic_cast<const CheckpointEvent*>(m));
  }

  return os;
}

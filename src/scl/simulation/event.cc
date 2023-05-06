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

namespace {

using Evt = scl::sim::Event;

auto EventTypeToString(Evt::Type type) {
  if (type == Evt::Type::START) {
    return "START";
  }

  if (type == Evt::Type::STOP) {
    return "STOP";
  }

  if (type == Evt::Type::SEND) {
    return "SEND";
  }

  if (type == Evt::Type::RECV) {
    return "RECV";
  }

  if (type == Evt::Type::HAS_DATA) {
    return "HAS_DATA";
  }

  if (type == Evt::Type::OUTPUT) {
    return "OUTPUT";
  }

  if (type == Evt::Type::SLEEP) {
    return "SLEEP";
  }

  if (type == Evt::Type::SEGMENT_BEGIN) {
    return "SEGMENT_BEGIN";
  }

  if (type == Evt::Type::SEGMENT_END) {
    return "SEGMENT_END";
  }

  if (type == Evt::Type::CHECKPOINT) {
    return "CHECKPOINT";
  }

  if (type == Evt::Type::PACKET_SEND) {
    return "PACKET_SEND";
  }

  if (type == Evt::Type::PACKET_RECV) {
    return "PACKET_RECV";
  }

  // if (type == scl::Measurement::Type::CLOSE)
  return "CLOSE";
}

void WriteClose(std::ostream& os, const scl::sim::NetworkEvent* m) {
  os << " [Local=" << m->LocalParty() << ", Remote=" << m->RemoteParty() << "]";
}

void WriteSend(std::ostream& os, const scl::sim::NetworkDataEvent* m) {
  os << " ["
     << "Sender=" << m->LocalParty() << ", Receiver=" << m->RemoteParty()
     << ", Amount=" << m->DataAmount() << "]";
}

void WriteRecv(std::ostream& os, const scl::sim::NetworkDataEvent* m) {
  os << " ["
     << "Receiver=" << m->LocalParty() << ", Sender=" << m->RemoteParty()
     << ", Amount=" << m->DataAmount() << "]";
}

void WritePacketRecv(std::ostream& os, const scl::sim::PacketRecvEvent* m) {
  os << " ["
     << "Receiver=" << m->LocalParty() << ", Sender=" << m->RemoteParty()
     << ", Amount=" << m->DataAmount() << ", Blocking=" << std::boolalpha
     << m->Blocking() << "]";
}

void WriteSegment(std::ostream& os, const scl::sim::SegmentEvent* m) {
  const auto name = m->Name();
  if (name.empty()) {
    os << " [Unnamed segment]";
  } else {
    os << " [Name=" << name << "]";
  }
}

void WriteHasData(std::ostream& os, const scl::sim::HasDataEvent* m) {
  os << " [Local=" << m->LocalParty() << ", Remote=" << m->RemoteParty()
     << ", DataAvailable=" << std::boolalpha << m->HadData() << "]";
}

void WriteCheckpoint(std::ostream& os, const scl::sim::CheckpointEvent* m) {
  os << " [" << m->Message() << "]";
}

}  // namespace

std::ostream& scl::sim::operator<<(std::ostream& os, const Evt* m) {
  using namespace std::chrono;
  const auto t = m->EventType();
  os << EventTypeToString(t) << " at ";
  os << duration<double, std::milli>(m->Timestamp()).count();
  os << " ms";
  if (m->Offset() > util::Time::Duration::zero()) {
    os << " [Offset=";
    os << duration<double, std::milli>(m->Offset()).count();
    os << " ms]";
  }

  if (t == Evt::Type::SEGMENT_BEGIN || t == Evt::Type::SEGMENT_END) {
    WriteSegment(os, dynamic_cast<const SegmentEvent*>(m));
  }

  if (t == Evt::Type::CLOSE) {
    WriteClose(os, dynamic_cast<const NetworkEvent*>(m));
  }

  if (t == Evt::Type::SEND || t == Evt::Type::PACKET_SEND) {
    WriteSend(os, dynamic_cast<const NetworkDataEvent*>(m));
  }

  if (t == Evt::Type::RECV) {
    WriteRecv(os, dynamic_cast<const NetworkDataEvent*>(m));
  }

  if (t == Evt::Type::PACKET_RECV) {
    WritePacketRecv(os, dynamic_cast<const PacketRecvEvent*>(m));
  }

  if (t == Evt::Type::HAS_DATA) {
    WriteHasData(os, dynamic_cast<const HasDataEvent*>(m));
  }

  if (t == Evt::Type::CHECKPOINT) {
    WriteCheckpoint(os, dynamic_cast<const CheckpointEvent*>(m));
  }

  return os;
}

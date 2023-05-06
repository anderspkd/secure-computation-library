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

#include "scl/net/channel.h"

void scl::net::Channel::Send(const scl::net::Packet& packet) {
  const auto size_size = sizeof(net::Packet::SizeType);
  unsigned char size_buf[size_size];
  net::Packet::SizeType packet_size = packet.Size();
  std::memcpy(size_buf, &packet_size, size_size);
  Send(size_buf, size_size);
  Send(packet.Get(), packet.Size());
}

std::optional<scl::net::Packet> scl::net::Channel::Recv(bool block) {
  const auto size_size = sizeof(net::Packet::SizeType);
  unsigned char size_buf[size_size];
  net::Packet::SizeType packet_size;

  if (block) {
    Recv(size_buf, size_size);
  } else {
    if (HasData()) {
      Recv(size_buf, size_size);
    } else {
      return {};
    }
  }

  std::memcpy(&packet_size, size_buf, size_size);

  net::Packet p(packet_size);
  Recv(p.Get(), packet_size);
  p.SetWritePtr(packet_size);
  return p;
}

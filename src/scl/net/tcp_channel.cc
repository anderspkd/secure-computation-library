/**
 * @file tcp_channel.cc
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#include "scl/net/tcp_channel.h"

#include "scl/net/tcp_utils.h"

void scl::TcpChannel::Close() {
  if (!mAlive) return;

  const auto err = scl::details::CloseSocket(mSocket);
  if (err < 0) {
    SCL_THROW_SYS_ERROR("error closing socket");
  }
  mAlive = false;
}

void scl::TcpChannel::Send(const unsigned char* src, std::size_t n) {
  std::size_t rem = n;
  std::size_t offset = 0;

  while (rem > 0) {
    auto sent = scl::details::WriteToSocket(mSocket, src + offset, rem);
    if (sent < 0) {
      SCL_THROW_SYS_ERROR("write failed");
    }
    rem -= sent;
    offset += sent;
  }
}

int scl::TcpChannel::Recv(unsigned char* dst, std::size_t n) {
  std::size_t rem = n;
  std::size_t offset = 0;

  while (rem > 0) {
    auto recv = scl::details::ReadFromSocket(mSocket, dst + offset, rem);
    if (!recv) {
      break;
    }
    if (recv < 0) {
      SCL_THROW_SYS_ERROR("read failed");
    }

    rem -= recv;
    offset += recv;
  }

  return n - rem;
}

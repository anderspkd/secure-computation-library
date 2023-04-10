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

#include "scl/net/threaded_sender.h"

#include <future>

scl::net::ThreadedSenderChannel::ThreadedSenderChannel(int socket)
    : m_channel(TcpChannel(socket)) {
  m_sender = std::async(std::launch::async, [&]() {
    while (true) {
      auto data = m_send_buffer.Peek();
      if (!m_channel.Alive()) {
        break;
      }
      m_channel.Send(data.data(), data.size());
      m_send_buffer.PopFront();
    }
  });
}

void scl::net::ThreadedSenderChannel::Close() {
  m_channel.Close();
  unsigned char stop_signal = 1;
  Send(&stop_signal, 1);
  m_sender.wait();
}

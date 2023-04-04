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
    : mChannel(TcpChannel(socket)) {
  mSender = std::async(std::launch::async, [&]() {
    while (true) {
      auto data = mSendBuffer.Peek();
      if (!mChannel.Alive()) {
        break;
      }
      mChannel.Send(data.data(), data.size());
      mSendBuffer.PopFront();
    }
  });
}

void scl::net::ThreadedSenderChannel::Close() {
  mChannel.Close();
  unsigned char stop_signal = 1;
  Send(&stop_signal, 1);
  mSender.wait();
}

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

#ifndef SCL_NET_THREADED_SENDER_H
#define SCL_NET_THREADED_SENDER_H

#include <cstddef>
#include <future>

#include "scl/net/channel.h"
#include "scl/net/shared_deque.h"
#include "scl/net/tcp_channel.h"

namespace scl::net {

/**
 * @brief A decorator for TcpChannel which does Send calls in a separate thread
 *
 * The purpose of this class is to avoid situations where calls to Send may
 * block, for example if we're trying to send more that what can fit in the TCP
 * window.
 */
class ThreadedSenderChannel final : public Channel {
 public:
  /**
   * @brief Create a new threaded sender channel.
   * @param socket an open socket used to construct scl::TcpChannel
   */
  ThreadedSenderChannel(int socket);

  /**
   * @brief Destroying a ThreadedSenderChannel closes the connection.
   */
  ~ThreadedSenderChannel() {
    Close();
  };

  void Close() override;

  void Send(const unsigned char* src, std::size_t n) override {
    m_send_buffer.PushBack({src, src + n});
  };

  std::size_t Recv(unsigned char* dst, std::size_t n) override {
    return m_channel.Recv(dst, n);
  };

  bool HasData() override {
    return m_channel.HasData();
  };

 private:
  TcpChannel<> m_channel;
  SharedDeque<std::vector<unsigned char>> m_send_buffer;
  std::future<void> m_sender;
};

}  // namespace scl::net

#endif  // SCL_NET_THREADED_SENDER_H

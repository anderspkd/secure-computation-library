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

#include "scl/net/network.h"

#include <cstdint>
#include <exception>
#include <ios>
#include <memory>
#include <system_error>
#include <thread>

#include "scl/coro/coroutine.h"
#include "scl/coro/future.h"
#include "scl/net/channel.h"
#include "scl/net/config.h"
#include "scl/net/loopback.h"
#include "scl/net/sys_iface.h"
#include "scl/net/tcp_channel.h"
#include "scl/net/tcp_utils.h"

using namespace scl;
using namespace std::chrono_literals;

namespace {

template <typename SYS>
coro::Task<void> writePartyId(net::SocketType socket, std::uint32_t party_id) {
  SYS::write(socket, &party_id, sizeof(std::uint32_t));
  co_return;
}

template <typename SYS>
coro::Task<std::uint32_t> readPartyId(net::SocketType socket) {
  std::uint32_t party_id;
  while (true) {
    auto read = SYS::read(socket, &party_id, sizeof(std::uint32_t));
    if (read < 0) {
      const auto err = SYS::getError();
      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await [sock = socket]() {
          return net::details::pollSocket(sock, POLLIN);
        };
      }
    } else {
      break;
    }
  }
  co_return party_id;
}

struct SocketAndId {
  net::SocketType socket;
  std::size_t id;
};

template <typename SYS>
coro::Task<SocketAndId> acceptConnection(net::SocketType server_socket) {
  while (true) {
    if (net::details::pollSocket(server_socket, POLLIN)) {
      auto conn = net::details::acceptConnection(server_socket);
      net::details::markSocketNonBlocking(conn.socket);

      auto id = co_await readPartyId<SYS>(conn.socket);

      co_return {conn.socket, id};
    } else {
      co_await 100ms;
    }
  }
}

template <typename SYS>
coro::Task<SocketAndId> establishConnection(net::Party party,
                                            std::size_t my_id) {
  std::size_t attempts = 100;  // max attempts.

  while (attempts > 0) {
    net::SocketType socket = -1;

    socket = net::details::connectAsClient(party.hostname, (int)party.port);
    // TODO: What errors to retry on?

    attempts--;

    if (socket == -1) {
      co_await 100ms;
    } else {
      net::details::markSocketNonBlocking(socket);
      co_await writePartyId<SYS>(socket, my_id);
      co_return {socket, party.id};
    }
  }

  throw std::runtime_error("could not establish connection to party");
}

}  // namespace

coro::Task<net::Network> net::Network::create(const NetworkConfig& config) {
  std::vector<std::shared_ptr<Channel>> channels(config.networkSize());

  const std::size_t id = config.id();
  const std::size_t n = config.networkSize();

  channels[id] = LoopbackChannel::create();

  std::vector<coro::Task<SocketAndId>> tasks;

  const auto me = config.party(id);
  auto server_socket = details::createServerSocket((int)me.port, 128);
  details::markSocketNonBlocking(server_socket);
  for (std::size_t i = 0; i < n; ++i) {
    if (i < id) {
      tasks.emplace_back(
          establishConnection<details::SysIFace>(config.party(i), me.id));
    } else if (i > id) {
      tasks.emplace_back(acceptConnection<details::SysIFace>(server_socket));
    }
  }

  std::vector<SocketAndId> sais = co_await coro::batch(std::move(tasks));

  details::SysIFace::close(server_socket);

  for (const SocketAndId& sai : sais) {
    channels[sai.id] = std::make_shared<TcpChannel<>>(sai.socket);
  }

  co_return Network{channels, config.id()};
}

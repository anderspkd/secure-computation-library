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

#include "scl/net/tcp/network.h"

#include <cstdint>
#include <memory>

#include "./syscalls.h"
#include "./utils.h"
#include "scl/coro/runtime.h"
#include "scl/net/channel.h"
#include "scl/net/config.h"
#include "scl/net/loopback.h"
#include "scl/net/tcp/channel.h"

namespace {

scl::Task<void> writePartyId(int socket, std::uint32_t party_id) {
  scl::details::sys_call::write(socket, &party_id, sizeof(std::uint32_t));
  co_return;
}

scl::Task<std::uint32_t> readPartyId(int socket) {
  std::uint32_t party_id;
  while (true) {
    auto read =
        scl::details::sys_call::read(socket, &party_id, sizeof(std::uint32_t));
    if (read < 0) {
      const auto err = scl::details::sys_call::getError();
      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await [sock = socket]() {
          return scl::details::pollSocket(sock, POLLIN);
        };
      }
    } else {
      break;
    }
  }
  co_return party_id;
}

struct SocketAndId {
  int socket;
  std::size_t id;
};

scl::Task<SocketAndId> acceptConnection(int server_socket) {
  using namespace std::chrono_literals;
  while (true) {
    if (scl::details::pollSocket(server_socket, POLLIN)) {
      auto conn = scl::details::acceptConnection(server_socket);
      scl::details::markSocketNonBlocking(conn.socket);

      auto id = co_await readPartyId(conn.socket);

      co_return {conn.socket, id};
    } else {
      co_await 100ms;
    }
  }
}

scl::Task<SocketAndId> establishConnection(scl::ConnectionInfo party,
                                           std::size_t my_id) {
  using namespace std::chrono_literals;
  std::size_t attempts = 100;  // max attempts.

  while (attempts > 0) {
    int socket = -1;

    socket = scl::details::connectAsClient(party.hostname, (int)party.port);
    // TODO: What errors to retry on?

    attempts--;

    if (socket == -1) {
      co_await 100ms;
    } else {
      scl::details::markSocketNonBlocking(socket);
      co_await writePartyId(socket, my_id);
      co_return {socket, party.id};
    }
  }

  throw std::runtime_error("could not establish connection to party");
}

}  // namespace

scl::Task<scl::Network> scl::createTcpNetwork(const NetworkConfig& config) {
  std::vector<std::shared_ptr<Channel>> channels(config.networkSize());

  const std::size_t id = config.id();
  const std::size_t n = config.networkSize();

  channels[id] = LoopbackChannel::create();

  std::vector<Task<SocketAndId>> tasks;

  const auto me = config.party(id);
  auto server_socket = details::createServerSocket((int)me.port, 128);
  details::markSocketNonBlocking(server_socket);
  for (std::size_t i = 0; i < n; ++i) {
    if (i < id) {
      tasks.emplace_back(establishConnection(config.party(i), me.id));
    } else if (i > id) {
      tasks.emplace_back(acceptConnection(server_socket));
    }
  }

  std::vector<SocketAndId> sais = co_await batch(std::move(tasks));

  details::sys_call::close(server_socket);

  for (const SocketAndId& sai : sais) {
    channels[sai.id] = std::make_shared<TcpChannel>(sai.socket);
  }

  co_return Network{channels, config.id()};
}

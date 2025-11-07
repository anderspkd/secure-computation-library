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

#include <algorithm>
#include <coroutine>
#include <cstdint>
#include <memory>

#include "./syscalls.h"
#include "./utils.h"
#include "scl/coro/runtime.h"
#include "scl/net/channel.h"
#include "scl/net/loopback.h"
#include "scl/net/tcp/channel.h"

using namespace scl;

namespace {

Task<void> writePartyId(int socket, std::uint32_t party_id) {
  details::sys_call::write(socket, &party_id, sizeof(std::uint32_t));
  co_return;
}

Task<std::uint32_t> readPartyId(int socket) {
  std::uint32_t party_id;
  while (true) {
    auto read =
        details::sys_call::read(socket, &party_id, sizeof(std::uint32_t));
    if (read < 0) {
      const auto err = details::sys_call::getError();
      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await
            [sock = socket]() { return details::pollSocket(sock, POLLIN); };
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

Task<SocketAndId> acceptConnection(int server_socket) {
  using namespace std::chrono_literals;
  while (true) {
    if (details::pollSocket(server_socket, POLLIN)) {
      auto conn = details::acceptConnection(server_socket);
      details::markSocketNonBlocking(conn.socket);

      auto id = co_await readPartyId(conn.socket);

      co_return {conn.socket, id};
    } else {
      co_await 100ms;
    }
  }
}

Task<SocketAndId> establishConnection(ConnectionInfo party, std::size_t my_id) {
  using namespace std::chrono_literals;
  std::size_t attempts = 100;  // max attempts.

  while (attempts > 0) {
    int socket = -1;

    socket = details::connectAsClient(party.hostname, (int)party.port);
    // TODO: What errors to retry on?

    attempts--;

    if (socket == -1) {
      co_await 100ms;
    } else {
      details::markSocketNonBlocking(socket);
      co_await writePartyId(socket, my_id);
      co_return {socket, party.id};
    }
  }

  throw std::runtime_error("could not establish connection to party");
}

class BatchTask final {
 public:
  BatchTask(std::vector<Task<SocketAndId>>&& tasks)
      : m_tasks(std::move(tasks)), m_runtime(nullptr) {}

  bool await_ready() const noexcept {
    return std::all_of(m_tasks.begin(), m_tasks.end(), [](const auto& task) {
      return task.ready();
    });
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) {
    for (auto& task : m_tasks) {
      task.setRuntime(m_runtime);
      m_runtime->schedule(task.m_handle);
    }
    m_runtime->schedule(handle, [this]() { return this->await_ready(); });

    return m_runtime->next();
  }

  std::vector<SocketAndId> await_resume() {
    std::vector<SocketAndId> results;
    for (const auto& task : m_tasks) {
      results.emplace_back(task.result());
    }
    return results;
  }

  void setRuntime(Runtime* runtime) {
    m_runtime = runtime;
  }

 private:
  std::vector<Task<SocketAndId>> m_tasks;
  Runtime* m_runtime;
};

}  // namespace

Task<Network> scl::createTcpNetwork(const NetworkConfig& config) {
  std::vector<std::shared_ptr<Channel>> channels(config.networkSize());

  const std::size_t id = config.id();
  const std::size_t n = config.networkSize();

  channels[id] = details::LoopbackChannel::create();

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

  std::vector<SocketAndId> sais = co_await BatchTask(std::move(tasks));

  details::sys_call::close(server_socket);

  for (const SocketAndId& sai : sais) {
    channels[sai.id] = std::make_shared<details::TcpChannel>(sai.socket);
  }

  co_return Network{channels, config.id()};
}

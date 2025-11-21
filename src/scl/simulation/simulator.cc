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

#include "scl/simulation/simulator.h"

#include <algorithm>
#include <memory>

#include "scl/net/loopback.h"
#include "scl/protocol.h"
#include "scl/simulation/channel.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/runtime.h"
#include "scl/simulation/transport.h"
#include "scl/time.h"

using namespace scl;

namespace {

// This class holds all the protocols we're going to simulate, and suspends
// until all of them finish. The main reason for this class existing is to
// assign an initial pid (party identifier) to each protocol coroutine.
class ProtocolBatch final {
 public:
  ProtocolBatch(std::vector<Task<void>>&& protocols)
      : m_protocols(std::move(protocols)) {}

  // ready when all protocols are ready, i.e., have finished running.
  bool await_ready() const noexcept {
    return std::all_of(m_protocols.begin(),
                       m_protocols.end(),
                       [](const Task<void>& task) { return task.ready(); });
  }

  // The coroutine passed here is the "simulate" function further down in this
  // file. Before the "simulate" function suspends, we schedule all the
  // protocols to run (using the current runtime); and before doing that, we
  // make sure that each protocol object gets their own party ID. This allows
  // the runtime to tell which coroutine belongs to what party.
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> coroutine) {
    details::SimulatorRuntime* rt =
        dynamic_cast<details::SimulatorRuntime*>(m_runtime);

    for (std::size_t pid = 0; pid < m_protocols.size(); pid++) {
      m_protocols[pid].setRuntime(m_runtime);
      rt->scheduleWithId(m_protocols[pid].m_handle, pid);
    }

    m_runtime->schedule(coroutine, [this]() { return await_ready(); });
    return m_runtime->next();
  }

  // need to call result() on all protocols in order to re-throw any uncaught
  // exceptions.
  void await_resume() {
    std::for_each(m_protocols.begin(),
                  m_protocols.end(),
                  [](const Task<void>& task) { task.result(); });
  }

  void setRuntime(Runtime* runtime) {
    m_runtime = runtime;
  }

 private:
  std::vector<Task<void>> m_protocols;
  Runtime* m_runtime;
};

// Main per-party protocol loop.
Task<void> runProtocol(std::unique_ptr<Protocol> protocol,
                       details::Context ctx,
                       Env env) {
  // Executing a protocol for a party goes more or less as follows:
  // 1. Emit a START event
  // 2. Emit a BEGIN event (signals the beginning of a Protocol)
  // 3. start this party's clock so we can measure how long it takes to run.
  // 4. run this party's protocol.
  // 5. emit an END event (signals the end of a protocol)
  // 6. handle outputs, if any.
  // 7. If current protocol output a next protocol, go back to 2 (with the new
  //    protocol).
  // 8. Emit a STOP event.

  try {
    ctx.addEvent<StartEvent>();

    while (protocol) {
      ctx.addEvent<BeginEvent>(ctx.lastEvent()->time(), protocol->name());

      ctx.startClock();
      auto next = co_await protocol->run(env);
      const auto et = ctx.elapsedTime();

      ctx.addEvent<EndEvent>(et, protocol->name());

      if (next.output.has_value()) {
        // TODO: handle output
      }

      // This will suspend this party, allowing someone else to run. It's not
      // really needed, but (hopefully) it ensures a more "fair" execution
      // order.
      //
      // TODO: Is this needed?
      co_await []() { return true; };

      protocol = std::move(next.next);
    }

    ctx.addEvent<StopEvent>(ctx.lastEvent()->time());

  } catch (std::exception& e) {
    // all exceptions are caught and discarded, but we make sure to record an
    // event.
    ctx.addEvent<KilledEvent>(ctx.lastEvent()->time(), e.what());
  }
}

// create the channels for a party.
std::vector<std::shared_ptr<Channel>> createChannels(
    details::Context ctx,
    std::shared_ptr<details::Transport> transport,
    std::size_t local_pid,
    std::size_t n) {
  std::vector<std::shared_ptr<Channel>> channels;
  channels.reserve(n);

  for (std::size_t remote_pid = 0; remote_pid < n; remote_pid++) {
    if (remote_pid == local_pid) {
      channels.emplace_back(details::LoopbackChannel::create());
    } else {
      ChannelId id(local_pid, remote_pid);
      channels.emplace_back(
          details::SimulatedChannel::create(id, ctx, transport));
    }
  }

  return channels;
}

// create the networks used by all parties.
std::vector<Network> createNetworks(details::SimulatorContext& sim_ctx) {
  const auto n = sim_ctx.numberOfParties();
  auto transport = std::make_shared<details::Transport>(sim_ctx);
  std::vector<Network> networks;
  networks.reserve(n);

  for (std::size_t pid = 0; pid < n; pid++) {
    networks.emplace_back(
        createChannels(sim_ctx.getContext(pid), transport, pid, n),
        pid);
  }

  return networks;
}

// Ensures that parties get a sensible value when reading their clock in a
// simulation.
class FakeClock final : public Clock {
 public:
  FakeClock(details::Context ctx) : m_ctx(ctx) {}

  Time::Duration read() const override {
    return m_ctx.elapsedTime();
  }

 private:
  details::Context m_ctx;
};

// performs setup stuff for the simulation
Task<void> simulate(std::vector<std::unique_ptr<Protocol>>&& protocols,
                    details::SimulatorContext& sim_ctx) {
  std::vector<Task<void>> protocol_tasks;
  std::vector<Network> networks = createNetworks(sim_ctx);

  for (std::size_t pid = 0; pid < protocols.size(); pid++) {
    const auto ctx = sim_ctx.getContext(pid);
    protocol_tasks.emplace_back(
        runProtocol(std::move(protocols[pid]),
                    ctx,
                    Env{networks[pid], std::make_unique<FakeClock>(ctx)}));
  }

  // runs the simulation.
  co_await ProtocolBatch(std::move(protocol_tasks));
}

}  // namespace

Simulator::Result Simulator::run(
    std::vector<std::unique_ptr<Protocol>>&& protocols,
    NetworkParams network_params) {
  if (!protocols.empty()) {
    auto sim_ctx = details::SimulatorContext::create(protocols.size(),
                                                     network_params,
                                                     std::move(m_hooks));
    auto runtime = std::make_unique<details::SimulatorRuntime>(sim_ctx);

    runtime->run(simulate(std::move(protocols), sim_ctx));
    return sim_ctx.toResult();
  }

  return Simulator::Result({});
}

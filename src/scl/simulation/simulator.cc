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

#include "scl/simulation/simulator.h"

#include <algorithm>
#include <coroutine>
#include <exception>

#include <bits/ranges_algo.h>

#include "scl/coro/coroutine.h"
#include "scl/net/loopback.h"
#include "scl/simulation/cancellation.h"
#include "scl/simulation/channel.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/runtime.h"
#include "scl/util/time.h"

using namespace scl;

namespace {

std::shared_ptr<net::Channel> createChannel(
    std::size_t i,
    std::size_t j,
    sim::details::GlobalContext& ctx,
    std::shared_ptr<sim::details::Transport> transport) {
  sim::ChannelId cid{i, j};
  return std::make_shared<sim::details::SimulatedChannel>(cid,
                                                          ctx.view(i),
                                                          transport);
}

// creates the networks used in this simulation.
std::vector<net::Network> createNetworks(std::size_t n,
                                         sim::details::GlobalContext& ctx) {
  auto transport = std::make_shared<sim::details::Transport>();
  std::vector<net::Network> networks;
  networks.reserve(n);

  for (std::size_t i = 0; i < n; i++) {
    std::vector<std::shared_ptr<net::Channel>> channels;
    channels.reserve(n);
    for (std::size_t j = 0; j < n; j++) {
      channels.emplace_back(createChannel(i, j, ctx, transport));
    }
    networks.emplace_back(channels, i);
  }

  return networks;
}

struct ClockImpl final : public proto::Clock {
  ClockImpl(const sim::details::GlobalContext::LocalContext& view)
      : view(view) {}

  util::Time::Duration read() const override {
    return view.elapsedTime();
  }

  sim::details::GlobalContext::LocalContext view;
};

auto createClock(const sim::details::GlobalContext::LocalContext& view) {
  return std::make_unique<ClockImpl>(view);
}

struct EnvAndCtx {
  proto::Env env;
  sim::details::GlobalContext::LocalContext view;
};

std::vector<EnvAndCtx> createEnvs(sim::details::GlobalContext& global_ctx) {
  const std::size_t n = global_ctx.number_of_parties;
  auto networks = createNetworks(n, global_ctx);
  std::vector<EnvAndCtx> envs;
  envs.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    auto view = global_ctx.view(i);
    envs.emplace_back(
        EnvAndCtx{proto::Env{networks[i], createClock(view)}, view});
  }

  return envs;
}

coro::Task<void> runProtocol(std::size_t id,
                             sim::Manager* manager,
                             std::unique_ptr<proto::Protocol> protocol,
                             EnvAndCtx&& env) {
  // A protocol is run for as long as all of the following is true:
  //  - it's output result contains another protocol to run;
  //  - it does not produce an uncaught exception;
  //  - it has not been cancelled.
  //
  // Running the protocol generates zero or more events, which is ultimately
  // what is the interesting stuff that we're interested in. The events are
  // generated (roughly speaking) in the following order:
  //  1. START
  //  2. Repeat as long as protocol->run().next != nullptr:
  //    2.1. PROTOCOL_BEGIN
  //    2.2. RECV, SEND, CLOSE, HAS_DATA, SLEEP ...}
  //    2.3. OUTPUT, in case the protocol generated output
  //    2.4. PROTOCOL_END
  //  3. STOP
  //
  // If any point (with two exceptions, listed below) a hook is run which
  // cancels the _current_ party (i.e, this party), then a CANCELLED event is
  // produced and the function returns; if an exception is thrown, a KILLED
  // event is produced with the exception's message and the function returns.

  auto& view = env.view;

  try {
    view.recordEvent(sim::Event::start());

    while (protocol) {
      const auto name = protocol->name();

      view.recordEvent(
          sim::Event::protocolBegin(view.lastEventTimestamp(), name));

      // start the clock of the party. This ensures that any time spent
      // book-keeping does not go towards the total running time of the party.
      view.startClock();
      auto next = co_await protocol->run(env.env);

      const auto et = view.elapsedTime();

      if (next.result.has_value()) {
        manager->handleProtocolOutput(id, next.result);
        view.recordEvent(sim::Event::output(et));
      }

      view.recordEvent(sim::Event::protocolEnd(et, name));

      protocol = std::move(next.next_protocol);
    }

    view.recordEvent(sim::Event::stop(view.lastEventTimestamp()));

    // We could keep running, however by suspending here we can allow a
    // different party to run. This is especially important if the protocol we
    // are running does not contain any suspension points.
    co_await []() { return true; };

  } catch (sim::details::CancellationException& /* ignored */) {
    // the simulation was cancelled by this party, so we just stop here.
    view.recordEvent(sim::Event::cancelled(view.lastEventTimestamp()));
  } catch (std::exception& e) {
    // something went wrong, so we mark the protocol as dead and stop.
    view.recordEvent(sim::Event::killed(view.lastEventTimestamp(), e.what()));
  }

  co_return;
}

// Helper class that runs the protocols we are simulating. This class behaves
// very similar to sim::Batch, but with a specialized await_suspend.
class SimBatch final {
 public:
  SimBatch(std::vector<coro::Task<void>>&& tasks,
           sim::details::GlobalContext& gctx)
      : m_tasks(std::move(tasks)), m_gctx(gctx) {}

  bool await_ready() const noexcept {
    // keep running until all non-cancelled coroutines have completed.
    for (std::size_t i = 0; i < m_tasks.size(); i++) {
      if (!m_gctx.cancellation_map.at(i) && !m_tasks[i].ready()) {
        return false;
      }
    }

    return true;
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> coroutine) {
    sim::details::SimulatorRuntime* srt =
        dynamic_cast<sim::details::SimulatorRuntime*>(m_runtime);
    for (std::size_t i = 0; i < m_tasks.size(); i++) {
      m_tasks[i].setRuntime(m_runtime);
      srt->scheduleWithId(m_tasks[i].m_handle, i);
    }

    m_runtime->schedule(coroutine, [this]() { return await_ready(); });

    return m_runtime->next();
  }

  void await_resume() {
    for (const auto& t : m_tasks) {
      t.result();
    }
  }

  void setRuntime(coro::Runtime* runtime) noexcept {
    m_runtime = runtime;
  }

 private:
  std::vector<coro::Task<void>> m_tasks;
  sim::details::GlobalContext& m_gctx;

  coro::Runtime* m_runtime;
};

coro::Task<void> runProtocols(
    std::vector<std::unique_ptr<proto::Protocol>>&& protocols,
    sim::details::GlobalContext& global_ctx,
    sim::Manager* manager) {
  std::vector<coro::Task<void>> protocol_runs;

  std::vector<EnvAndCtx> envs = createEnvs(global_ctx);
  for (std::size_t i = 0; i < protocols.size(); i++) {
    protocol_runs.emplace_back(
        runProtocol(i, manager, std::move(protocols[i]), std::move(envs[i])));
  }
  co_await SimBatch(std::move(protocol_runs), global_ctx);
}

}  // namespace

void sim::simulate(std::unique_ptr<sim::Manager> manager) {
  auto protocol = manager->protocol();

  // do nothing in case the caller (for whatever reason) wanted to simulate an
  // empty protocol.
  if (!protocol.empty()) {
    auto ctx = details::GlobalContext::create(protocol.size(),
                                              manager->networkConfiguration(),
                                              std::move(manager->m_hooks));
    auto runtime = std::make_unique<details::SimulatorRuntime>(ctx);

    // const auto start = util::Time::now();

    runtime->run(runProtocols(std::move(protocol), ctx, manager.get()));

    // const auto sim_dur = util::Time::now() - start;
    // std::cout << "simulation took " << util::timeToMillis(sim_dur) << "ms\n";

    for (std::size_t party_id = 0; party_id < ctx.traces.size(); party_id++) {
      manager->handleSimulatorOutput(party_id, ctx.traces[party_id]);
    }
  }
}

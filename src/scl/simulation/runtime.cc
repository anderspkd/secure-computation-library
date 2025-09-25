#include "scl/simulation/runtime.h"

#include <coroutine>

void scl::SimulatorRuntime::schedule(std::coroutine_handle<> coroutine,
                                     std::function<bool()>&& predicate) {
  m_tq.emplace_back(coroutine, std::move(predicate), m_current_pid);
}

void scl::SimulatorRuntime::schedule(std::coroutine_handle<> coroutine,
                                     Time::Duration delay) {
  auto ctx = m_sim_ctx.getContext(m_current_pid);
  const auto et = ctx.lastEvent()->time();
  ctx.addEvent<SleepEvent>(et + delay, delay);
  this->deschedule(coroutine);
}

void scl::SimulatorRuntime::scheduleWithId(std::coroutine_handle<> coroutine,
                                           std::size_t pid) {
  m_tq.emplace_back(coroutine, []() { return true; }, pid);
}

void scl::SimulatorRuntime::deschedule(std::coroutine_handle<> coroutine) {
  m_tq.remove_if(
      [&coroutine](const Coro& coro) { return coro.coroutine == coroutine; });
}

std::coroutine_handle<> scl::SimulatorRuntime::next() {
  auto beg = m_tq.begin();
  const auto end = m_tq.end();

  while (beg != end) {
    const auto [coroutine, predicate, pid] = *beg;

    if (predicate()) {
      m_tq.erase(beg);
      m_current_pid = pid;

      if (pid != MANAGE_PID) {
        m_sim_ctx.getContext(m_current_pid).startClock();
      }

      return coroutine;
    }

    ++beg;
  }

  return std::noop_coroutine();
}

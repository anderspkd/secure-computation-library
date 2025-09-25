#ifndef SCL_SIMULATION_RUNTIME_H
#define SCL_SIMULATION_RUNTIME_H

#include <coroutine>

#include "scl/coro/runtime.h"
#include "scl/simulation/context.h"

namespace scl {

class SimulatorRuntime final : public Runtime {
 private:
  static const std::size_t MANAGE_PID = static_cast<std::size_t>(-1);

 public:
  SimulatorRuntime(SimulatorContext& sim_ctx) : m_sim_ctx(sim_ctx) {}

  void schedule(std::coroutine_handle<> coroutine,
                std::function<bool()>&& predicate) override;

  void schedule(std::coroutine_handle<> coroutine,
                Time::Duration delay) override;

  void scheduleWithId(std::coroutine_handle<> coroutine, std::size_t pid);

  void deschedule(std::coroutine_handle<> coroutine) override;

  bool taskQueueEmpty() const override {
    return m_tq.empty();
  }

  std::coroutine_handle<> next() override;

 private:
  struct Coro {
    std::coroutine_handle<> coroutine;
    std::function<bool()> predicate;
    std::size_t pid;
  };

  SimulatorContext& m_sim_ctx;
  std::size_t m_current_pid;
  std::list<Coro> m_tq;
};

}  // namespace scl

#endif  // SCL_SIMULATION_RUNTIME_H

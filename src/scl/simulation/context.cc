#include "scl/simulation/context.h"

#include "scl/simulation/event.h"

using namespace scl;

SimulatorContext SimulatorContext::create(
    NetworkDescription network_desc,
    std::vector<Simulator::SimulationHook>&& hooks) {
  SimulatorContext ctx{network_desc};

  ctx.m_events.resize(network_desc.size());
  ctx.m_clocks.resize(network_desc.size());
  ctx.m_hooks = std::move(hooks);

  return ctx;
}

void SimulatorContext::runHooks(std::size_t pid, Event* event) {
  for (auto& [trigger, hook] : m_hooks) {
    // if the trigger was specified, then the hook is only run if the current
    // event matches the trigger type.
    if (trigger.has_value() && (trigger.value() == event->type())) {
      hook(pid, event);
    } else {
      // otherwise the hook is run unconditionally.
      hook(pid, event);
    }
  }
}

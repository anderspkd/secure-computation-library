#ifndef SCL_SIMULATION_SIMULATOR_H
#define SCL_SIMULATION_SIMULATOR_H

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

#include "scl/protocol.h"
#include "scl/simulation/event.h"
#include "scl/simulation/network_description.h"

namespace scl {

/**
 * @brief Concept for protocol builder objects.
 *
 * A "ProtocolBuilder" is any type which, when invoked, returns an std::vector
 * of Protocol objects.
 */
template <typename BUILDER>
concept ProtocolBuilder = requires(BUILDER builder) {
  { builder() } -> std::convertible_to<std::vector<std::unique_ptr<Protocol>>>;
};

/**
 * @brief The simulator.
 */
class Simulator final {
 public:
  struct SimulationHook {
    using HookType = std::function<void(std::size_t, Event*)>;

    std::optional<EventType> trigger;
    HookType hook;
  };

  template <ProtocolBuilder BUILDER>
  void run(BUILDER& builder, NetworkDescription network_desc) {
    auto protocol = builder();
    if (protocol.size() != network_desc.size()) {
      throw std::logic_error("protocols do not match network definition");
    }
    run(std::move(protocol), network_desc);
  }

  template <typename HOOK>
    requires(std::convertible_to<HOOK, SimulationHook::HookType>)
  void addHook(EventType trigger, HOOK hook) {
    m_hooks.emplace_back(trigger, hook);
  }

  template <typename HOOK>
    requires(std::convertible_to<HOOK, SimulationHook::HookType>)
  void addHook(HOOK hook) {
    m_hooks.emplace_back({}, hook);
  }

 private:
  std::vector<SimulationHook> m_hooks;

  void run(std::vector<std::unique_ptr<Protocol>>&& protocols,
           NetworkDescription network_desc);
};

}  // namespace scl

#endif  // SCL_SIMULATION_SIMULATOR_H

#ifndef SCL_PROTOCOL_H
#define SCL_PROTOCOL_H

#include <any>

#include "scl/coro/task.h"
#include "scl/net/network.h"
#include "scl/time.h"

namespace scl {

struct Env {
  Network network;
  std::unique_ptr<Clock> clock;
};

struct Result;

struct Protocol {
  virtual ~Protocol() {}

  constexpr static const char* DEFAULT_NAME = "N/A";

  virtual Task<Result> run(Env& env) const = 0;

  virtual std::string name() const {
    return DEFAULT_NAME;
  }
};

struct Result {
  static Result done() {
    return Result{.next = nullptr, .output = {}};
  }

  static Result done(std::any output) {
    return Result{.next = nullptr, .output = output};
  }

  static Result nextStep(std::unique_ptr<Protocol> next) {
    return Result{.next = std::move(next), .output = {}};
  }

  static Result nextStep(std::unique_ptr<Protocol> next, std::any output) {
    return Result{.next = std::move(next), .output = output};
  }

  std::unique_ptr<Protocol> next;
  std::any output;
};

}  // namespace scl

#endif  // SCL_PROTOCOL_H

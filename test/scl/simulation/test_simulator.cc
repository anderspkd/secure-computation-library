/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
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

#include <algorithm>
#include <catch2/catch.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "../protocol/beaver.h"
#include "scl/math/fp.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/simulation/config.h"
#include "scl/simulation/manager.h"
#include "scl/simulation/result.h"
#include "scl/simulation/simulator.h"
#include "scl/ss/additive.h"
#include "scl/util/prg.h"

using namespace scl;
using namespace std::chrono_literals;

using FF = math::Fp<61>;
using Parties = std::vector<std::unique_ptr<proto::Protocol>>;

namespace {

template <typename T, typename... Ts>
std::unique_ptr<proto::Protocol> CreateParty(Ts&&... init_args) {
  return std::make_unique<T>(std::forward(init_args)...);
}

auto RecvTimeDefaultConf(std::size_t n) {
  static const auto dft = sim::ChannelConfig::Default();
  return sim::ComputeRecvTime(dft, n);
}

}  // namespace

struct SimpleSendRecvProtocol {
  struct Sender final : public proto::Protocol {
    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      net::Packet p;
      p << (std::size_t)123;
      p << (int)-100;
      env.network.Other()->Send(p);
      env.network.Close();
      return nullptr;
    }
  };

  struct Receiver final : public proto::Protocol {
    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      auto p = env.network.Other()->Recv(true);

      if (!p.has_value()) {
        throw std::runtime_error("expected data");
      }
      auto& v = p.value();

      is_correct = v.Read<std::size_t>() == 123;
      is_correct &= v.Read<int>() == -100;

      env.network.Close();
      return nullptr;
    };

    std::any Output() const override {
      return is_correct;
    }

    bool is_correct = false;
  };
};

namespace {

void VerifySendRecvProtocolResult(const std::vector<sim::Result>& result) {
  REQUIRE(result.size() == 2);

  const auto& r0 = result[0];
  REQUIRE(r0.SegmentNames().size() == 1);
  REQUIRE(r0.SegmentNames()[0] == proto::Protocol::DEFAULT_NAME);

  const auto et0 = r0.ExecutionTime();
  REQUIRE(et0.Size() == 1);
  REQUIRE(et0.Samples()[0] < 1ms);

  const auto et1 = result[1].ExecutionTime();
  REQUIRE(et1.Size() == 1);
  const auto bytes_recv =
      sizeof(int) + sizeof(std::size_t) + sizeof(net::Packet::SizeType);
  REQUIRE(et1.Samples()[0] < RecvTimeDefaultConf(bytes_recv) + 1ms);
}

}  // namespace

TEST_CASE("Simulate SimpleSendRecvProtocol", "[sim]") {
  Parties p;
  p.emplace_back(CreateParty<SimpleSendRecvProtocol::Sender>());
  p.emplace_back(CreateParty<SimpleSendRecvProtocol::Receiver>());

  const auto result = sim::Simulate(std::move(p));
  VerifySendRecvProtocolResult(result);
}

TEST_CASE("Simulate SimpleSendRecvProtocol reverse", "[sim]") {
  Parties p;
  p.emplace_back(CreateParty<SimpleSendRecvProtocol::Receiver>());
  p.emplace_back(CreateParty<SimpleSendRecvProtocol::Sender>());

  const auto result = sim::Simulate(std::move(p));
  VerifySendRecvProtocolResult({result[1], result[0]});
}

namespace {

void VerifyType(std::shared_ptr<sim::Event> event, sim::Event::Type type) {
  REQUIRE(event->EventType() == type);
}

void VerifyTypeString(std::stringstream& ss,
                      const std::string& event_type_str) {
  std::string line;
  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith(event_type_str));
}

}  // namespace

TEST_CASE("Simulate SimpleSendRecvProtocol trace", "[sim]") {
  Parties p;
  p.emplace_back(CreateParty<SimpleSendRecvProtocol::Sender>());
  p.emplace_back(CreateParty<SimpleSendRecvProtocol::Receiver>());

  const auto result = sim::Simulate(std::move(p));

  SECTION("Sender") {
    const auto& sender_trace = result[0].Trace(0);

    VerifyType(sender_trace[0], sim::Event::Type::START);
    VerifyType(sender_trace[1], sim::Event::Type::SEGMENT_BEGIN);
    VerifyType(sender_trace[2], sim::Event::Type::PACKET_SEND);
    VerifyType(sender_trace[3], sim::Event::Type::CLOSE);  // to self
    VerifyType(sender_trace[4], sim::Event::Type::CLOSE);  // to other
    VerifyType(sender_trace[5], sim::Event::Type::SEGMENT_END);
    VerifyType(sender_trace[6], sim::Event::Type::STOP);

    std::stringstream ss;
    result[0].WriteTrace(ss, 0);
    VerifyTypeString(ss, "START");
    VerifyTypeString(ss, "SEGMENT_BEGIN");
    VerifyTypeString(ss, "PACKET_SEND");
    VerifyTypeString(ss, "CLOSE");
    VerifyTypeString(ss, "CLOSE");
    VerifyTypeString(ss, "SEGMENT_END");
    VerifyTypeString(ss, "STOP");
  }

  SECTION("Receiver") {
    const auto& receiver_trace = result[1].Trace(0);

    VerifyType(receiver_trace[0], sim::Event::Type::START);
    VerifyType(receiver_trace[1], sim::Event::Type::SEGMENT_BEGIN);
    VerifyType(receiver_trace[2], sim::Event::Type::PACKET_RECV);
    VerifyType(receiver_trace[3], sim::Event::Type::CLOSE);  // to self
    VerifyType(receiver_trace[4], sim::Event::Type::CLOSE);  // to other
    VerifyType(receiver_trace[5], sim::Event::Type::OUTPUT);
    VerifyType(receiver_trace[6], sim::Event::Type::SEGMENT_END);
    VerifyType(receiver_trace[7], sim::Event::Type::STOP);

    std::stringstream ss;
    result[1].WriteTrace(ss, 0);

    VerifyTypeString(ss, "START");
    VerifyTypeString(ss, "SEGMENT_BEGIN");
    VerifyTypeString(ss, "PACKET_RECV");
    VerifyTypeString(ss, "CLOSE");
    VerifyTypeString(ss, "CLOSE");
    VerifyTypeString(ss, "OUTPUT");
    VerifyTypeString(ss, "SEGMENT_END");
    VerifyTypeString(ss, "STOP");
  }
}

TEST_CASE("Simulate null protocol", "[sim]") {
  Parties p;
  p.emplace_back(nullptr);

  const auto result = sim::Simulate(std::move(p));

  REQUIRE(result.size() == 1);
  const auto trace = result[0].Trace(0);
  REQUIRE(trace[0]->EventType() == sim::Event::Type::START);
  REQUIRE(trace[1]->EventType() == sim::Event::Type::STOP);
}

struct PingPongProtocol {
  struct Ping final : public proto::Protocol {
    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      unsigned char data[] = {'a', 'b', 'c'};
      env.network.Other()->Send(data, 3);
      env.thread_ctx->Sleep(1000);
      return std::make_unique<Pong>();
    }
    std::string Name() const override {
      return "Ping";
    }
  };

  struct Pong final : public proto::Protocol {
    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      unsigned char data[3] = {0};
      env.network.Other()->Recv(data, 3);
      bool good = data[0] == 'a' && data[1] == 'b' && data[2] == 'c';
      env.clock->Checkpoint(good ? "yay" : "boo");
      return std::make_unique<Ping>();
    }
    std::string Name() const override {
      return "Pong";
    }
  };
};

struct PingPongManager final : public sim::Manager {
  PingPongManager(std::size_t replications) : sim::Manager(replications) {}

  std::vector<std::unique_ptr<proto::Protocol>> Protocol() override {
    Parties p;
    p.emplace_back(std::make_unique<PingPongProtocol::Ping>());
    p.emplace_back(std::make_unique<PingPongProtocol::Pong>());
    return p;
  }

  bool Terminate(std::size_t party_id,
                 const sim::Context::View& view) override {
    const auto latest_time = view.Trace(party_id).back()->Timestamp();
    return latest_time > 10s;
  }
};

TEST_CASE("Simulate PingPongProtocol", "[sim]") {
  auto m = std::make_unique<PingPongManager>(1);
  const auto result = sim::Simulate(std::move(m));

  const auto last_event_p0 = result[0].Trace(0).back();
  VerifyType(last_event_p0, sim::Event::Type::KILLED);
  REQUIRE(last_event_p0->Timestamp() >= 10000ms);

  const auto last_event_p1 = result[1].Trace(0).back();
  VerifyType(last_event_p1, sim::Event::Type::KILLED);
  REQUIRE(last_event_p1->Timestamp() >= 10000ms);
}

TEST_CASE("Simulate PingPongProtocol trace", "[sim]") {
  auto m = std::make_unique<PingPongManager>(1);
  const auto result = sim::Simulate(std::move(m));

  std::stringstream ss;
  std::string line;
  result[0].WriteTrace(ss, 0, "Ping");

  // ping/pong runs for 10 iterations
  for (std::size_t i = 0; i < 10; ++i) {
    VerifyTypeString(ss, "SEGMENT_BEGIN");
    VerifyTypeString(ss, "SEND");
    VerifyTypeString(ss, "SLEEP");
    VerifyTypeString(ss, "SEGMENT_END");
  }
}

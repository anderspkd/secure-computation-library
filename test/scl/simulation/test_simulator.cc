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

#include <catch2/catch.hpp>
#include <chrono>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <thread>

#include "../protocol/beaver.h"
#include "scl/math/fp.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/simulation/config.h"
#include "scl/simulation/result.h"
#include "scl/simulation/simulator.h"
#include "scl/ss/additive.h"
#include "scl/util/prg.h"

using namespace scl;

using FF = math::Fp<61>;
using namespace std::chrono_literals;

namespace {

template <typename T>
void ApproxDuration(util::Time::Duration d, T v, T b) {
  if (v > d) {
    REQUIRE(v - d <= b);
  } else {
    REQUIRE(d - v <= b);
  }
}

std::size_t KB(std::size_t bytes) {
  return 1000 * bytes;
}

std::size_t MB(std::size_t bytes) {
  return 1000 * KB(bytes);
}

}  // namespace

TEST_CASE("ComputeRecvTime default config", "[sim]") {
  // https://wintelguy.com/wanperf.pl
  // parameters:
  //  Link bandwidth (Mbit/s):            1
  //  RTT (millisecond):                  100
  //  Packet loss (%):                    0
  //  MTU (Byte):                         1500
  //  L1/L2 frame overhead (Byte):        0  <-- not accounted for in scl
  //  TCP/IP (v4) header overhead (Byte): 40
  //  TCP window (RWND) size (Byte):      65536
  //  File size (MByte):                  1

  const auto cfg = sim::SimulatedNetworkConfig::Default();
  const auto tenMB = MB(10);
  const auto t = sim::ComputeRecvTime(cfg, tenMB);
  ApproxDuration(t, 82s, 1s);
}

TEST_CASE("ComputeRecvTime lossy", "[sim]") {
  const auto cfg =
      sim::SimulatedNetworkConfig::Builder().PackageLoss(0.001).Build();
  const auto tenMB = MB(10);
  const auto t = sim::ComputeRecvTime(cfg, tenMB);
  ApproxDuration(t, 82s, 1s);
}

/**
 * @brief Protocol with many rounds.
 *
 * This protocol runs for 101 steps. Each of the first 100 steps send a single
 * int while the last step receives all the data sent. Each party will send some
 * data to the next party.
 *
 * The output of the protocol is a boolean indicating if the received data
 * matched the data sent.
 */
struct LotsOfDataProtocol {
  struct Two final : proto::Protocol {
    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      std::vector<int> data(100);
      auto& network = env.network;
      const auto id = network.MyId();
      const auto rid = id == 0 ? network.Size() - 1 : id - 1;
      for (std::size_t i = 0; i < 100; ++i) {
        auto p = network.Previous()->Recv();
        if (!p.has_value()) {
          output = false;
        } else {
          std::size_t rid_c = p.value().Read<int>();
          output &= rid_c == rid + i;
        }
      }
      network.Previous()->Close();
      return nullptr;
    };

    std::any Output() const override {
      return output;
    }

    bool output = true;
  };

  struct One final : proto::Protocol {
    One(int counter = 0) : counter(counter){};
    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      auto& network = env.network;
      const auto id = network.MyId();
      net::Packet p;
      p << (int)(id + counter);
      network.Next()->Send(p);
      if (counter > 100) {
        return std::make_unique<Two>();
      }
      return std::make_unique<One>(counter + 1);
    };

    int counter;
  };
};

TEST_CASE("Simulation many", "[sim]") {
  const auto n_parties = 10;

  std::vector<bool> outputs(n_parties, false);

  const auto output_cb = [&outputs](std::size_t id, const std::any& output) {
    REQUIRE(output.has_value());
    outputs[id] = std::any_cast<bool>(output);
  };

  std::vector<std::unique_ptr<proto::Protocol>> parties;
  for (std::size_t i = 0; i < n_parties; ++i) {
    parties.emplace_back(std::make_unique<LotsOfDataProtocol::One>());
  }

  const auto r =
      sim::Simulate(std::move(parties), sim::DefaultConfigCreator(), output_cb);

  for (std::size_t i = 0; i < n_parties; ++i) {
    REQUIRE(outputs[i]);
  }
}

/**
 * @brief Simple protocol where one party sends a boolean to another party.
 */
struct SendRecvProtocol {
  struct Sender final : proto::Protocol {
    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      net::Packet p;
      p << true;
      env.network.Other()->Send(p);
      return nullptr;
    }
  };

  struct Receiver final : proto::Protocol {
    Receiver(const std::function<void()>& cb) : cb(cb){};

    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      cb();
      auto p = env.network.Other()->Recv();
      output = p.has_value() && p.value().Read<bool>();
      return nullptr;
    }

    std::any Output() const override {
      return output;
    }

    bool output;
    std::function<void()> cb;
  };
};

TEST_CASE("Simulation result trace", "[sim]") {
  std::vector<std::unique_ptr<proto::Protocol>> parties;
  parties.emplace_back(std::make_unique<SendRecvProtocol::Receiver>([]() {}));
  parties.emplace_back(std::make_unique<SendRecvProtocol::Sender>());

  const auto r = sim::Simulate(std::move(parties), sim::DefaultConfigCreator());

  std::stringstream ss;
  r[0].WriteTrace(ss, 0);

  std::string line;

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("START"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("SEGMENT_BEGIN"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("PACKET_RECV"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("OUTPUT"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("SEGMENT_END"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("STOP"));

  REQUIRE_THROWS_MATCHES(r[0].WriteTrace(ss, 42),
                         std::invalid_argument,
                         Catch::Matchers::Message("invalid iteration"));

  ss.str("");
  r[0].WriteTrace(ss, 0, proto::Protocol::kDefaultName);

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("SEGMENT_BEGIN"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("PACKET_RECV"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("OUTPUT"));

  std::getline(ss, line);
  REQUIRE_THAT(line, Catch::Matchers::StartsWith("SEGMENT_END"));
}

TEST_CASE("Simulation odd/even iterations", "[sim]") {
  sim::DataMeasurement m_even;
  sim::DataMeasurement m_odd;

  // Cannot use SECTION here as m_even somehow gets overwritten with garbage...

  {
    const auto creator = []() {
      std::vector<std::unique_ptr<proto::Protocol>> parties;
      parties.emplace_back(std::make_unique<SendRecvProtocol::Sender>());
      parties.emplace_back(
          std::make_unique<SendRecvProtocol::Receiver>([]() {}));
      return parties;
    };

    const auto r = sim::Simulate(creator, sim::DefaultConfigCreator(), 2);
    m_even = r[0].TransferAmounts().recv;
  }

  {
    const auto creator = []() {
      std::vector<std::unique_ptr<proto::Protocol>> parties;
      parties.emplace_back(std::make_unique<SendRecvProtocol::Sender>());
      parties.emplace_back(
          std::make_unique<SendRecvProtocol::Receiver>([]() {}));
      return parties;
    };

    const auto r = sim::Simulate(creator, sim::DefaultConfigCreator(), 3);
    m_odd = r[0].TransferAmounts().recv;
  }

  REQUIRE(m_even.Mean() == m_odd.Mean());
  REQUIRE(m_even.Median() == m_odd.Median());
  REQUIRE(m_even.Min() == m_odd.Min());
  REQUIRE(m_even.Max() == m_odd.Max());
}

TEST_CASE("Simulation receive out-of-order", "[sim]") {
  SECTION("Receive before send") {
    int called = 0;
    const auto cb = [&called]() { called++; };

    std::vector<std::unique_ptr<proto::Protocol>> parties;
    parties.emplace_back(std::make_unique<SendRecvProtocol::Receiver>(cb));
    parties.emplace_back(std::make_unique<SendRecvProtocol::Sender>());

    const auto r =
        sim::Simulate(std::move(parties), sim::DefaultConfigCreator());

    // receive bool (1 byte) + packet size (4 bytes).
    REQUIRE(r[0].TransferAmounts().recv.Max() == 5.0);
    ApproxDuration(r[0].ExecutionTime().Max(), 100ms, 1ms);
    ApproxDuration(r[1].ExecutionTime().Max(), 1ms, 1ms);

    // Ensure that receiver was called twice.
    REQUIRE(called == 2);
  }

  SECTION("Send before receive") {
    int called = 0;
    const auto cb = [&called]() { called++; };

    std::vector<std::unique_ptr<proto::Protocol>> parties_;
    parties_.emplace_back(std::make_unique<SendRecvProtocol::Sender>());
    parties_.emplace_back(std::make_unique<SendRecvProtocol::Receiver>(cb));

    const auto r_ =
        sim::Simulate(std::move(parties_), sim::DefaultConfigCreator());

    // sent bool (1 byte) + packet size (4 bytes).
    REQUIRE(r_[0].TransferAmounts().sent.Max() == 5.0);
    ApproxDuration(r_[1].ExecutionTime().Max(), 100ms, 1ms);
    ApproxDuration(r_[0].ExecutionTime().Max(), 1ms, 1ms);

    REQUIRE(called == 1);
  }
}

/**
 * @brief Two party protocol that uses HasData.
 *
 * This protocol captures both failure cases for simulating a HasData call. Both
 * failure cases arise
 */
struct HasDataProtocol {
  struct Bob;
  struct Alice;

  struct Alice final : public proto::Protocol {
    Alice(bool sleep, bool exit_early, bool send)
        : sleep(sleep), exit_early(exit_early), send(send){};

    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      if (sleep) {
        env.thread_ctx->Sleep(50);
      }
      if (send) {
        env.network.Other()->Send(42);
      }
      if (exit_early) {
        return nullptr;
      }
      return std::make_unique<Dummy>();
    }

    struct Dummy final : public proto::Protocol {
      std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
        (void)env;
        return nullptr;
      }
    };

    bool sleep;
    bool exit_early;
    bool send;
  };

  struct Bob final : public proto::Protocol {
    Bob(bool sleep) : sleep(sleep){};

    std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
      if (sleep) {
        env.thread_ctx->Sleep(50);
      }
      output = env.network.Other()->HasData();
      return nullptr;
    }

    std::any Output() const override {
      return output;
    }

    bool output;
    bool sleep;
  };
};

namespace {

bool TestHasData(bool alice_sleep,
                 bool alice_exit_early,
                 bool alice_send,
                 bool bob_sleep,
                 bool bob_before_alice = false) {
  using Alice = HasDataProtocol::Alice;
  using Bob = HasDataProtocol::Bob;

  std::optional<bool> output;
  const auto cb = [&output](std::size_t id, const std::any& o) {
    (void)id;
    output = std::any_cast<bool>(o);
  };

  std::vector<std::unique_ptr<proto::Protocol>> parties;
  if (bob_before_alice) {
    parties.emplace_back(std::make_unique<Bob>(bob_sleep));
    parties.emplace_back(
        std::make_unique<Alice>(alice_sleep, alice_exit_early, alice_send));
  } else {
    parties.emplace_back(
        std::make_unique<Alice>(alice_sleep, alice_exit_early, alice_send));
    parties.emplace_back(std::make_unique<Bob>(bob_sleep));
  }

  const auto r =
      sim::Simulate(std::move(parties), sim::DefaultConfigCreator(), cb);

  if (output.has_value()) {
    return output.value();
  }
  FAIL("output did not have a value");
  return false;
}

}  // namespace

TEST_CASE("Simulation HasData", "[sim]") {
  SECTION("Alice never sends data") {
    const auto result = TestHasData(true, false, false, false);
    REQUIRE_FALSE(result);
  }

  SECTION("Alice sends data after Bob") {
    const auto result = TestHasData(true, false, true, false);
    REQUIRE_FALSE(result);
  }

  SECTION("Alice sends data before Bob") {
    const auto result = TestHasData(false, false, true, true);
    REQUIRE(result);
  }

  SECTION("Bob before Alice") {
    const auto result = TestHasData(false, true, true, true, true);
    REQUIRE(result);
  }
}

TEST_CASE("Simulation Beaver", "[sim]") {
  auto creator = []() {
    std::vector<std::unique_ptr<proto::Protocol>> parties;
    auto prg = util::PRG::Create();
    auto xs = ss::AdditiveShare(FF(42), 2, prg);
    auto ys = ss::AdditiveShare(FF(11), 2, prg);
    auto ts = test::RandomTriple<FF>(prg);

    parties.emplace_back(test::BeaverMul<FF>::Create(xs[0], ys[0], ts[0]));
    parties.emplace_back(test::BeaverMul<FF>::Create(xs[1], ys[1], ts[1]));

    return parties;
  };

  const auto result = sim::Simulate(creator, sim::DefaultConfigCreator(), 10);

  SECTION("segment names") {
    const auto segment_names = result[0].SegmentNames();
    std::vector<std::string> expected_names = {"init", "finalize"};
    REQUIRE_THAT(segment_names, Catch::Matchers::Contains(expected_names));
  }

  SECTION("running time") {
    ApproxDuration(result[0].ExecutionTime().Mean(), 113ms, 1ms);
    ApproxDuration(result[1].ExecutionTime().Mean(), 113ms, 1ms);

    ApproxDuration(result[0].ExecutionTime("init").Mean(), 1ms, 1ms);
    ApproxDuration(result[1].ExecutionTime("finalize").Mean(), 113ms, 1ms);
  }

  SECTION("transfer") {
    // Each party sends 8 bytes (vec lenghts) + 100 field elements of 8 bytes
    // each.
    REQUIRE(result[0].TransferAmounts().sent.Mean() == 8 + 4 * 100 * 8);
    REQUIRE(result[1].TransferAmounts().sent.Mean() == 8 + 4 * 100 * 8);

    REQUIRE(result[0].TransferAmounts().recv.Mean() == 8 + 4 * 100 * 8);
    REQUIRE(result[1].TransferAmounts().recv.Mean() == 8 + 4 * 100 * 8);

    // The simple beaver protocol sends is deterministic wrt. data amounts
    REQUIRE(result[0].TransferAmounts().recv.Min() ==
            result[0].TransferAmounts().recv.Max());

    REQUIRE(result[1].TransferAmounts().recv.Min() ==
            result[1].TransferAmounts().recv.Max());
  }
}

struct SinglePartyProtocol final : public proto::Protocol {
  SinglePartyProtocol(bool send = true) : send(send){};

  std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
    if (send) {
      std::vector<int> data(100, 42);
      net::Packet p;
      p << data;
      env.network.Party(0)->Send(p);
      return std::make_unique<SinglePartyProtocol>(false);
    }
    auto p = env.network.Party(0)->Recv();
    if (p.has_value()) {
      output = p.value().Read<std::vector<int>>();
    }
    return nullptr;
  }

  bool send;
  std::vector<int> output;
};

TEST_CASE("Simulation one party", "[sim]") {
  std::vector<std::unique_ptr<proto::Protocol>> party;
  party.push_back(std::make_unique<SinglePartyProtocol>());

  auto r = sim::Simulate(std::move(party), sim::DefaultConfigCreator());
  // The default configuration should ensure that communication locally
  // happens almost instantly
  ApproxDuration(r[0].ExecutionTime().Max(), 1ms, 1ms);
}

struct ChunkedRecvProtocol final : public proto::Protocol {
  std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
    if (env.network.MyId() == 0) {
      unsigned char buf[5] = {4, 5, 6, 7, 8};
      env.network.Other()->Send(buf, 3);
      env.network.Other()->Send(buf + 3, 2);
    } else {
      unsigned char buf[5] = {0};
      env.network.Other()->Recv(buf, 5);
      bool good = true;
      for (std::size_t i = 0; i < 5; ++i) {
        good &= (4 + i) == buf[i];
      }
      m_output = good;
    }
    return nullptr;
  }

  std::any Output() const override {
    return m_output;
  }

  bool m_output;
};

TEST_CASE("Simulation chunked receive", "[sim]") {
  bool correct = false;
  auto cb = [&correct](auto id, std::any output) {
    if (id == 1) {
      correct = std::any_cast<bool>(output);
    }
  };

  std::vector<std::unique_ptr<proto::Protocol>> parties;
  parties.emplace_back(std::make_unique<ChunkedRecvProtocol>());
  parties.emplace_back(std::make_unique<ChunkedRecvProtocol>());
  sim::Simulate(std::move(parties), sim::DefaultConfigCreator(), cb);

  REQUIRE(correct);
}

struct NonBlockRecvProtocol final : public proto::Protocol {
  NonBlockRecvProtocol(bool sleep = false) : sleep(sleep) {}

  std::unique_ptr<proto::Protocol> Run(proto::Env& env) override {
    if (env.network.MyId() == 0) {
      net::Packet p;
      p << 1 << 2 << 3;
      env.thread_ctx->Sleep(10);
      env.network.Party(1)->Send(p);
    } else {
      if (sleep) {
        env.thread_ctx->Sleep(20);
      }

      auto p = env.network.Party(0)->Recv(false);
    }
    return nullptr;
  }

  bool sleep;
};

TEST_CASE("Simulation non-block recv", "[sim]") {
  std::vector<std::unique_ptr<proto::Protocol>> p_no_data;
  p_no_data.emplace_back(std::make_unique<NonBlockRecvProtocol>());
  p_no_data.emplace_back(std::make_unique<NonBlockRecvProtocol>(false));

  const auto r0 =
      sim::Simulate(std::move(p_no_data), sim::DefaultConfigCreator());

  // execution is instant because no data is available.
  ApproxDuration(r0[1].ExecutionTime().Max(), 1ms, 1ms);

  std::vector<std::unique_ptr<proto::Protocol>> p_data;
  p_data.emplace_back(std::make_unique<NonBlockRecvProtocol>());
  p_data.emplace_back(std::make_unique<NonBlockRecvProtocol>(true));

  const auto r1 = sim::Simulate(std::move(p_data), sim::DefaultConfigCreator());

  // execution has to wait for the packet, because data was available.
  ApproxDuration(r1[1].ExecutionTime().Max(), 110ms, 10ms);
}

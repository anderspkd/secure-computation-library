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

#include "scl/simulation/channel.h"

#include <memory>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/simulator.h"
#include "scl/util/time.h"

using EventPtr = std::shared_ptr<scl::sim::Event>;

EventPtr scl::sim::SimulateClose(std::shared_ptr<Context> ctx, ChannelId id) {
  const auto lid = id.local;
  const auto trt = ctx->Checkpoint(lid);
  return std::make_shared<NetworkEvent>(Event::Type::CLOSE, trt, id);
}

#define SCL_LOCAL_COMP_BEGIN const auto scl__lcb = scl::util::Time::Now()
#define SCL_LOCAL_COMP_END scl::util::Time::Now() - scl__lcb

EventPtr scl::sim::SimulateSend(std::shared_ptr<Context> ctx,
                                ChannelId id,
                                const unsigned char* src,
                                std::size_t n) {
  SCL_LOCAL_COMP_BEGIN;

  ctx->Buffer(id)->Write(src, n);

  const auto local_comp_time = SCL_LOCAL_COMP_END;
  const auto exec_time = ctx->Checkpoint(id.local) - local_comp_time;

  auto event =
      std::make_shared<NetworkDataEvent>(Event::Type::SEND, exec_time, id, n);
  ctx->AddCandidateToRun(id.remote);
  ctx->AddWrite(id, n, exec_time);
  return event;
}

namespace {

scl::util::Time::Duration AdjustRecvTime(std::shared_ptr<scl::sim::Context> ctx,
                                         scl::sim::ChannelId id,
                                         scl::util::Time::Duration t,
                                         std::size_t n) {
  auto rem = n;

  while (rem > 0 && ctx->HasWrite(id)) {
    auto& w = ctx->NextWrite(id);
    scl::util::Time::Duration recv_time;
    if (w.amount > rem) {
      const auto delay =
          scl::sim::ComputeRecvTime(ctx->ChannelConfiguration(id), rem);
      recv_time = w.time + delay;
      w.amount -= rem;
      rem = 0;
    } else {
      const auto delay =
          scl::sim::ComputeRecvTime(ctx->ChannelConfiguration(id), w.amount);
      recv_time = w.time + delay;
      rem -= w.amount;
      ctx->DeleteWrite(id);
    }

    t = std::max(t, recv_time);
  }

  return t;
}

}  // namespace

EventPtr scl::sim::SimulateRecv(std::shared_ptr<Context> ctx,
                                ChannelId id,
                                unsigned char* dst,
                                std::size_t n) {
  SCL_LOCAL_COMP_BEGIN;

  if (ctx->Buffer(id)->Size() < n) {
    ctx->AddCandidateToRun(id.remote);
    throw SimulationFailure();
  }

  ctx->Buffer(id)->Read(dst, n);

  const auto local_comp_time = SCL_LOCAL_COMP_END;
  const auto exec_time = ctx->Checkpoint(id.local) - local_comp_time;
  const auto adjusted_time = AdjustRecvTime(ctx, id.Flip(), exec_time, n);

  return std::make_shared<NetworkDataEvent>(Event::Type::RECV,
                                            exec_time,
                                            adjusted_time - exec_time,
                                            id,
                                            n);
}

std::pair<bool, EventPtr> scl::sim::SimulateHasData(
    std::shared_ptr<Context> ctx,
    ChannelId id) {
  // The other party hasn't had a chance to run yet, so it's not possible to
  // determine if there's data available for us.
  if (ctx->Trace(id.remote).empty()) {
    ctx->AddCandidateToRun(id.remote);
    throw SimulationFailure("other party hasnt started yet");
  }

  // We determine if there is data available by inspecting the list of WriteOps
  // created by the remote party. Since each WriteOp has a timestamp, we can use
  // that to determine if the data would have arrived at us yet.
  //
  // The rules for what to return, and when to fail the simulation goes as
  // follows:
  //
  //  - WriteOp op exists such that op.amount > 0. This op corresponds to the
  //    data that we would receive the next time we call Recv on this channel.
  //
  //    If it is the case that
  //
  //      op.time + time_to_send_1_byte <= our_current_time,
  //
  //    then we can return has_data == true. Otherwise, we can return false.
  //    Note that, even if the remote party is behind is in time, we know that
  //    it is not possible for it to send data that we would receive earlier
  //    than the data connected to op.
  //
  //  - No WriteOp exists. In this case, we either return has_data == false, or
  //    we fail the simulation. We can return has_data == false if
  //
  //      remote_current_time - time_to_send_1_byte >= our_current_time
  //
  //    as we know that no Send that the remote party makes, would have arrived
  //    to us before now. On the other hand, if the above does not hold, then we
  //    cannot say for sure that the remote party might not send data that we
  //    would be able to receive now, and so we have to fail the simulation.

  // Time it takes for 1 byte to go from the remote party to us.
  const auto offset = ComputeRecvTime(ctx->ChannelConfiguration(id.Flip()), 1);

  // Go through each write op of the other party, and find the earliest one.
  const auto me_latest = ctx->Checkpoint(id.local);
  bool has_data = false;
  bool has_result = false;
  if (ctx->HasWrite(id.Flip())) {
    if (ctx->NextWrite(id.Flip()).time + offset <= me_latest) {
      has_data = true;
    } else {
      has_data = false;
      has_result = true;
    }
  }

  // Handle the case where no WriteOp existed at all. Here we will fail the
  // simulation if the remote party is too far behind us in time.
  if (!has_data && !has_result) {
    const auto other_latest = ctx->LatestTimestamp(id.remote) - offset;
    if (!ctx->HasTerminated(id.remote) && other_latest <= me_latest) {
      ctx->AddCandidateToRun(id.remote);
      throw SimulationFailure("no data, and we're ahead");
    }
  }

  const auto event = std::make_shared<HasDataEvent>(me_latest, id, has_data);
  return {has_data, event};
}

void scl::sim::Channel::Send(const scl::net::Packet& packet) {
  const auto packet_size = packet.Size();
  const auto size_size = sizeof(net::Packet::SizeType);

  // A packet is a size + content, which are sent separately.
  scl::net::Channel::Send(packet);

  // Sending the size and conte each generate a "SEND" event. These are removed
  // here, and replaced by a single "PACKET_SEND" event that is set to have
  // happened at the same time as the first event, and with an amount equal to
  // the sum of the two events.
  const auto data_event = m_ctx->PopLastEvent(m_id.local);
  const auto size_event = m_ctx->PopLastEvent(m_id.local);
  const auto event =
      std::make_shared<NetworkDataEvent>(Event::Type::PACKET_SEND,
                                         size_event->Timestamp(),
                                         m_id,
                                         size_size + packet_size);
  m_ctx->AddEvent(m_id.local, event);
}

namespace {

std::size_t GetDataAmount(scl::sim::Event* event) {
  return reinterpret_cast<scl::sim::NetworkDataEvent*>(event)->DataAmount();
}

}  // namespace

std::optional<scl::net::Packet> scl::sim::Channel::Recv(bool block) {
  // A packet is received a little differently, depending on whether it blocks
  // or not. If the recv is blocking, then we receive a size + content. If the
  // receive is non-blocking, then we first check if there's data before
  // receiving the size + content.

  auto p = net::Channel::Recv(block);

  if (block) {
    // Receive was blocking, so we need to remove the two last events,
    // corresponding to the receiving the size of the packet, and the packet's
    // content. The information in these two events is then turned into a
    // PACKET_RECV event.
    const auto data_event = m_ctx->PopLastEvent(m_id.local);
    const auto size_event = m_ctx->PopLastEvent(m_id.local);

    m_ctx->AddEvent(
        m_id.local,
        std::make_shared<sim::PacketRecvEvent>(
            size_event->Timestamp() - size_event->Offset(),
            size_event->Offset() + data_event->Offset(),
            m_id,
            GetDataAmount(data_event.get()) + GetDataAmount(size_event.get()),
            true));
  } else {
    // If the receive was non-blocking, then we either have one event (in case
    // there was no data to receive), or three (in case there was data to
    // receive).
    //
    // The extra event here, compared to the blocking case, is an event arising
    // from a call to HasData.
    if (p.has_value()) {
      const auto data_event = m_ctx->PopLastEvent(m_id.local);
      const auto size_event = m_ctx->PopLastEvent(m_id.local);
      const auto hd_event = m_ctx->PopLastEvent(m_id.local);

      const auto event = std::make_shared<sim::PacketRecvEvent>(
          hd_event->Timestamp(),
          size_event->Offset() + data_event->Offset(),
          m_id,
          GetDataAmount(data_event.get()) + GetDataAmount(size_event.get()),
          false);

      m_ctx->AddEvent(m_id.local, event);

    } else {
      const auto hd_event = m_ctx->PopLastEvent(m_id.local);

      m_ctx->AddEvent(
          m_id.local,
          std::make_shared<sim::PacketRecvEvent>(hd_event->Timestamp(),
                                                 util::Time::Duration::zero(),
                                                 m_id,
                                                 0,
                                                 false));
    }
  }

  return p;
}  // LCOV_EXCL_LINE

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

#include "scl/simulation/result.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/event.h"
#include "scl/simulation/measurement.h"
#include "scl/util/time.h"

using namespace scl;

namespace {

struct SentRecv {
  long double sent = 0;
  long double recv = 0;
};

using SentRecvMap = std::unordered_map<std::size_t, SentRecv>;

using CheckpointMap = std::unordered_map<std::string, util::Time::Duration>;

struct Segment {
  // sent/recv to other parties
  SentRecvMap sr;
  // execution time of the segment
  util::Time::Duration dur;
  // checkpoints found in the segment
  CheckpointMap checkpoints;
};

std::string GetNameFromSegmentEvent(std::shared_ptr<sim::Event> event) {
  return std::dynamic_pointer_cast<sim::SegmentEvent>(event)->Name();
}

using NamedSegment = std::pair<std::string, Segment>;

bool IsRecvEvent(std::shared_ptr<sim::NetworkDataEvent> ptr) {
  return ptr->EventType() == sim::Event::Type::RECV ||
         ptr->EventType() == sim::Event::Type::PACKET_RECV;
}

bool IsSendEvent(std::shared_ptr<sim::NetworkDataEvent> ptr) {
  return ptr->EventType() == sim::Event::Type::SEND ||
         ptr->EventType() == sim::Event::Type::PACKET_SEND;
}

/**
 * @brief Parse a segment from a simulation trace.
 * @param start a beginning iterator, pointing to a SEGMENT_BEGIN event.
 * @param end an end iterator.
 */
template <typename It>
NamedSegment ParseSegment(It start, const It end) {
  Segment seg;

  std::shared_ptr<sim::Event> event = *start;

  const auto name = GetNameFromSegmentEvent(event);
  seg.dur = event->Timestamp();

  start++;

  while (start < end) {
    event = *start;

    auto ne = std::dynamic_pointer_cast<sim::NetworkDataEvent>(event);

    if (ne != nullptr) {
      const auto id = ne->RemoteParty();
      if (IsRecvEvent(ne)) {
        seg.sr[id].recv += ne->DataAmount();
      }
      if (IsSendEvent(ne)) {
        seg.sr[id].sent += ne->DataAmount();
      }
    }

    if (event->EventType() == sim::Event::Type::CHECKPOINT) {
      const auto* ce = dynamic_cast<sim::CheckpointEvent*>(event.get());
      seg.checkpoints[ce->Id()] = ce->Timestamp();
    }

    if (event->EventType() == sim::Event::Type::SEGMENT_END) {
      seg.dur = event->Timestamp() - seg.dur;
      return {name, seg};
    }
    start++;
  }

  // we never saw a SEGMENT_END event, and now there's no more events, which
  // means the simulation trace was incomplete/malformed.
  throw std::logic_error("incomplete segment");
}

/**
 * @brief Adds two maps.
 *
 * Helper function for adding two maps:
 *
 * <ul>
 * <li>If an entry exists in \p m1 and \p m0, then the values from \p m1 are
 * added to those already in \p m0 </li>

 * <li>If an entry exists in \p m1 but not in \p m0, then the entry from \p m0
 * is added to \p m1 </li>
 * </ul>
 */
void UpdateSentRecv(SentRecvMap& m0, const SentRecvMap& m1) {
  for (const auto& [k, v] : m1) {
    if (m0.find(k) == m0.end()) {
      m0[k] = v;
    } else {
      m0[k].sent += v.sent;
      m0[k].recv += v.recv;
    }
  }
}

using SegmentMap = std::unordered_map<sim::Result::SegmentName, Segment>;

/**
 * @brief Merge segments by their name.
 *
 * This takes a list of name, segment pairs, and merges the information in the
 * segments that have the same name.
 */
SegmentMap MergeSegments(const std::vector<NamedSegment>& segments) {
  SegmentMap m;

  m[{}].dur = util::Time::Duration::zero();

  for (const auto& named_seg : segments) {
    const auto name = named_seg.first;
    const auto segm = named_seg.second;

    if (m.find(name) == m.end()) {
      m[name] = segm;
    } else {
      m[name].dur += segm.dur;
      UpdateSentRecv(m[name].sr, segm.sr);
      m[name].checkpoints.insert(segm.checkpoints.begin(),
                                 segm.checkpoints.end());
    }

    m[{}].dur += segm.dur;
    UpdateSentRecv(m[{}].sr, segm.sr);
  }

  return m;
}  // LCOV_EXCL_LINE

template <typename It>
void ValidateTraceHeadAndTail(It head, It tail) {
  if ((*head)->EventType() != sim::Event::Type::START) {
    throw std::logic_error("incomplete trace");
  }
  const auto last = (*tail)->EventType();
  if (last != sim::Event::Type::STOP && last != sim::Event::Type::KILLED) {
    throw std::logic_error("truncated trace");
  }
}

void AppendIfMissing(std::vector<std::string>& list,
                     const std::string& element) {
  if (std::find(list.begin(), list.end(), element) == list.end()) {
    list.emplace_back(element);
  }
}

}  // namespace

/**
 * @brief Create a result from a list of simulation traces.
 */
sim::Result sim::Result::Create(
    const std::vector<sim::SimulationTrace>& traces) {
  std::vector<SegmentMap> segments;

  for (const auto& trace : traces) {
    auto b = trace.begin();
    const auto e = trace.end();

    // sanity check
    ValidateTraceHeadAndTail(b, e - 1);

    // Extract each segment
    std::vector<NamedSegment> named_segments;
    while (b < e) {
      std::shared_ptr<sim::Event> event = *b;

      if (event->EventType() == sim::Event::Type::SEGMENT_BEGIN) {
        named_segments.emplace_back(ParseSegment(b, e));
      }

      b++;
    }

    // Merge segments by name
    segments.emplace_back(MergeSegments(named_segments));
  }

  std::vector<std::string> segment_names;
  std::unordered_map<SegmentName, SegmentMeasurement> segment_measurements;
  std::unordered_map<std::string, TimeMeasurement> checkpoints;

  for (const auto& seg_map : segments) {
    for (const auto& [seg_name, seg] : seg_map) {
      if (seg_name.has_value()) {
        // clang-tidy cannot see that we check if seg_name has a value above, so
        // disable the linter here to avoid false negatives.
        const auto v = seg_name.value();  // NOLINT
        AppendIfMissing(segment_names, v);
      }

      for (const auto& [s, c] : seg.checkpoints) {
        checkpoints[s].AddSample(c);
      }

      segment_measurements[seg_name].duration_m.AddSample(seg.dur);

      SentRecv total;
      for (const auto& [cid, sr] : seg.sr) {
        segment_measurements[seg_name].channels_m[cid].recv.AddSample(sr.recv);
        segment_measurements[seg_name].channels_m[cid].sent.AddSample(sr.sent);
        total.recv += sr.recv;
        total.sent += sr.sent;
      }

      segment_measurements[seg_name].send_recv_m.recv.AddSample(total.recv);
      segment_measurements[seg_name].send_recv_m.sent.AddSample(total.sent);
    }
  }

  return Result(traces, segment_measurements, checkpoints, segment_names);
}

std::vector<sim::Result> sim::Result::Create(
    const std::vector<std::vector<sim::SimulationTrace>>& traces) {
  const auto num_parties = traces[0].size();
  const auto num_replications = traces.size();

  std::vector<Result> results;
  results.reserve(num_parties);

  for (std::size_t i = 0; i < num_parties; ++i) {
    std::vector<SimulationTrace> traces_for_party;
    traces_for_party.reserve(num_replications);
    for (std::size_t j = 0; j < num_replications; ++j) {
      traces_for_party.emplace_back(traces[j][i]);
    }

    results.emplace_back(Create(traces_for_party));
  }

  return results;
}

namespace {

template <typename V>
std::vector<std::size_t> KeySet(const std::unordered_map<std::size_t, V>& map) {
  std::vector<std::size_t> keys;
  for (const auto& [key, val] : map) {
    (void)val;
    keys.emplace_back(key);
  }
  return keys;
}  // LCOV_EXCL_LINE

}  // namespace

std::vector<std::size_t> sim::Result::Interactions(
    const SegmentName& name) const {
  return KeySet(m_measurements.at(name).channels_m);
}

namespace {

template <typename It>
void WriteSegmentTrace(std::ostream& stream, It start, It end) {
  while (start != end) {
    stream << *(start++) << std::endl;
  }
}

}  // namespace

void sim::Result::WriteTrace(std::ostream& stream,
                             std::size_t replication,
                             const sim::Result::SegmentName& name) const {
  if (replication >= m_traces.size()) {
    throw std::invalid_argument("invalid replication");
  }

  if (!name.has_value()) {
    WriteSegmentTrace(stream,
                      m_traces[replication].begin(),
                      m_traces[replication].end());
  } else {
    const auto& segment_name = name.value();
    bool in_relevant_segment = false;

    for (const auto& e : m_traces[replication]) {
      if (in_relevant_segment) {
        stream << e << std::endl;
      }

      const auto s = std::dynamic_pointer_cast<SegmentEvent>(e);

      if (s != nullptr) {
        if (!in_relevant_segment &&
            s->EventType() == Event::Type::SEGMENT_BEGIN &&
            s->Name() == segment_name) {
          stream << e << std::endl;
          in_relevant_segment = true;
        }

        if (in_relevant_segment && s->EventType() == Event::Type::SEGMENT_END) {
          in_relevant_segment = false;
        }
      }
    }
  }
}

namespace {

template <typename K, typename V>
void WriteMap(std::ostream& stream, const std::unordered_map<K, V>& map);

void WriteObj(std::ostream& stream, const std::string& string) {
  stream << "\"" << string << "\"";
}

void WriteKey(std::ostream& stream, const std::string& name) {
  WriteObj(stream, name);
  stream << ":";
}

void WriteObj(std::ostream& stream, const std::size_t& val) {
  stream << val;
}

void WriteObj(std::ostream& stream, const long double& val) {
  stream << val;
}

void WriteObj(std::ostream& stream, const util::Time::Duration& d) {
  auto t = std::chrono::duration<long double, std::milli>(d).count();
  WriteObj(stream, t);
}

void WriteObj(std::ostream& stream, const std::optional<std::string>& opt) {
  if (opt.has_value()) {
    WriteObj(stream, opt.value());
  } else {
    stream << "null";
  }
}

template <typename T>
void WriteUnit(std::ostream& stream);

template <>
void WriteUnit<long double>(std::ostream& stream) {
  WriteObj(stream, std::string{"bytes"});
}

template <>
void WriteUnit<util::Time::Duration>(std::ostream& stream) {
  WriteObj(stream, std::string{"milliseconds"});
}

template <typename T>
void WriteList(std::ostream& stream, const std::vector<T>& items);

template <typename T>
void WriteObj(std::ostream& stream, const sim::Measurement<T>& m) {
  stream << "{";
  WriteKey(stream, "unit");
  WriteUnit<T>(stream);
  stream << ",";
  WriteKey(stream, "samples");
  WriteList(stream, m.Samples());
  stream << "}";
}

void WriteObj(std::ostream& stream, const sim::SendRecvMeasurement& srm) {
  stream << "{";
  WriteKey(stream, "sent");
  WriteObj(stream, srm.sent);
  stream << ",";
  WriteKey(stream, "recv");
  WriteObj(stream, srm.recv);
  stream << "}";
}

void WriteObj(std::ostream& stream, const sim::Result::SegmentMeasurement& m) {
  stream << "{";
  WriteKey(stream, "time");
  WriteObj(stream, m.duration_m);
  stream << ",";
  WriteKey(stream, "data");
  WriteObj(stream, m.send_recv_m);
  stream << ",";
  WriteKey(stream, "channels");
  WriteMap(stream, m.channels_m);
  stream << "}";
}

template <typename K, typename V>
void WriteObj(std::ostream& stream, const std::pair<const K, V>& pair) {
  stream << "{";
  WriteKey(stream, "key");
  WriteObj(stream, pair.first);
  stream << ",";
  WriteKey(stream, "value");
  WriteObj(stream, pair.second);
  stream << "}";
}

template <typename T>
void WriteList(std::ostream& stream, const std::vector<T>& items) {
  stream << "[";
  for (std::size_t i = 0; i < items.size(); ++i) {
    WriteObj(stream, items[i]);
    if (i < items.size() - 1) {
      stream << ",";
    }
  }
  stream << "]";
}

template <typename K, typename V>
void WriteMap(std::ostream& stream, const std::unordered_map<K, V>& map) {
  std::vector<std::pair<const K, V>> kvs(map.begin(), map.end());
  WriteList(stream, kvs);
}

}  // namespace

void sim::Result::Write(std::ostream& stream) const {
  stream << "{";

  WriteKey(stream, "names");
  WriteList(stream, m_segment_names);
  stream << ",";

  WriteKey(stream, "measurements");
  WriteMap(stream, m_measurements);
  stream << ",";

  WriteKey(stream, "checkpoints");
  WriteMap(stream, m_checkpoints);

  stream << "}" << std::endl;
}

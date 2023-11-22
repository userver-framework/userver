#pragma once

#include <userver/utils/statistics/metric_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::statistics {

enum class LogFileStates : size_t {
  Write = 0,
  Closed = 1,
};

struct LogFileState final {
  LogFileState(size_t val = static_cast<size_t>(LogFileStates::Write)) noexcept
      : state(val) {}

  LogFileState(const LogFileState& other) noexcept
      : state(other.state.load()) {}

  void SetStateToClosed() {
    state.store(static_cast<size_t>(LogFileStates::Closed));
  }

  void SetStateToWrite() {
    state.store(static_cast<size_t>(LogFileStates::Write));
  }

  std::atomic<size_t> state;
};

void DumpMetric(utils::statistics::Writer& writer, const LogFileState& stats);

}  // namespace logging::statistics

USERVER_NAMESPACE_END

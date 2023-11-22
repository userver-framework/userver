#include <logging/statistics/log_file_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::statistics {

void DumpMetric(utils::statistics::Writer& writer, const LogFileState& stats) {
  writer["log_file_state"] = stats.state.load();
}

}  // namespace logging::statistics

USERVER_NAMESPACE_END

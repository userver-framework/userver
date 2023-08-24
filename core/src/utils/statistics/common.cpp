#include <userver/utils/statistics/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

using seconds = std::chrono::seconds;
using minutes = std::chrono::minutes;

std::string DurationToString(seconds duration) {
  const auto min = std::chrono::floor<minutes>(duration);
  const auto sec = min - duration;

  std::string result;

  if (min.count()) result = std::to_string(min.count()) + "min";
  if (sec.count()) result += std::to_string(sec.count()) + "sec";

  return result;
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END

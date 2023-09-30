#include "time.hpp"

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

std::chrono::system_clock::time_point TransformToTimePoint(
    std::chrono::seconds seconds) {
  return std::chrono::system_clock::time_point{seconds};
}

std::optional<std::chrono::system_clock::time_point> TransformToTimePoint(
    std::optional<std::chrono::seconds> seconds) {
  if (!seconds) {
    return std::nullopt;
  }
  return TransformToTimePoint(seconds.value());
}

std::chrono::seconds TransformToSeconds(
    std::chrono::system_clock::time_point time_point) {
  return std::chrono::duration_cast<std::chrono::seconds>(
    time_point.time_since_epoch());
}

std::optional<std::chrono::seconds> TransformToSeconds(
    std::optional<std::chrono::system_clock::time_point> time_point) {
  if (!time_point) {
    return std::nullopt;
  }
  return TransformToSeconds(time_point.value());
};

}  // namespace telegram::bot

USERVER_NAMESPACE_END

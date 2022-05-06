#include "dynamic_debug.hpp"

#include <atomic>
#include <set>

#include <fmt/format.h>
#include <boost/algorithm/string.hpp>

#include <engine/task/task_context.hpp>
#include <userver/engine/run_in_coro.hpp>
#include <userver/logging/log_filepath.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

// E.g.: core/include/userver/logging/log.hpp
using RelativeLocation = utils::StrongTypedef<class RelLocTag, std::string>;

// E.g.: /home/user/uservices/userver/core/include/userver/logging/log.hpp
using AbsoluteLocation =
    utils::StrongTypedef<class AbsLocTag, std::string,
                         utils::StrongTypedefOps::kCompareTransparent>;

std::atomic<bool> dynamic_debug_enabled{false};

const std::string kRegisteredPrefixes[] = {
#ifdef USERVER_LOG_BUILD_PATH_BASE
    USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_BUILD_PATH_BASE),
#endif
#ifdef USERVER_LOG_SOURCE_PATH_BASE
    USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_SOURCE_PATH_BASE),
#endif
};

struct DynamicDebugData {
  std::set<AbsoluteLocation, std::less<>> enabled_locations_absolute;
  std::unordered_set<AbsoluteLocation> restricted_locations;
};

rcu::Variable<DynamicDebugData>& GetDynamicDebugData() {
  static rcu::Variable<DynamicDebugData> locations;
  return locations;
}

AbsoluteLocation ToAbsoluteLocation(const std::string& prefix,
                                    std::string_view location_relative) {
  return AbsoluteLocation{prefix + std::string(location_relative)};
}

RelativeLocation FromAbsoluteLocation(const AbsoluteLocation& abs_location) {
  std::string_view sv = abs_location.GetUnderlying();

  size_t pos = impl::PathBaseSize(sv);
  if (pos > 0) {
    return RelativeLocation{std::string{sv.substr(pos)}};
  }

  throw std::runtime_error(
      fmt::format("Failed to resolve absolute location '{}'", abs_location));
}

void DoInitDynamicDebugLog() {
  std::unordered_set<std::string> restrict_list;
  for (auto* ptr = impl::entry_slist; ptr != nullptr; ptr = ptr->next)
    restrict_list.insert(std::string{ptr->name});

  logging::RestrictDynamicDebugLocations(restrict_list);
}

}  // namespace

bool DynamicDebugShouldLog(std::string_view location) {
  // The hot code, used in the default logger

  // Called w/o location from ShouldLogStacktrace()
  if (location.empty()) return false;

  if (!dynamic_debug_enabled.load()) return false;

  auto ddl = GetDynamicDebugData().Read();
  const auto& enabled_locations = ddl->enabled_locations_absolute;
  auto it = enabled_locations.lower_bound(location);
  return it != enabled_locations.end() && *it == location;
}

bool DynamicDebugShouldLogRelative(std::string_view location_relative) {
  // Not a hot code
  auto ddl = GetDynamicDebugData().Read();

  for (const auto& prefix : kRegisteredPrefixes) {
    auto location = ToAbsoluteLocation(std::string(prefix), location_relative);
    if (!ddl->restricted_locations.empty() &&
        ddl->restricted_locations.count(location) == 0) {
      continue;
    }

    return DynamicDebugShouldLog(location.GetUnderlying());
  }

  throw std::runtime_error(
      "dynamic-debug-log: input Location is not registered");
}

void DoSetDynamicDebugLog(const std::string& prefix,
                          const std::string& location_relative, bool enable) {
  auto location = ToAbsoluteLocation(prefix, location_relative);

  auto ddl = GetDynamicDebugData().StartWrite();

  if (!ddl->restricted_locations.empty() &&
      ddl->restricted_locations.count(location) == 0) {
    throw std::runtime_error(
        "dynamic-debug-log: input Location is not registered");
  }

  if (enable) {
    ddl->enabled_locations_absolute.insert(location);
  } else {
    ddl->enabled_locations_absolute.erase(location);
  }
  size_t count = ddl->enabled_locations_absolute.size();
  ddl.Commit();

  if (!!count != dynamic_debug_enabled.load()) {
    dynamic_debug_enabled = !!count;
  }
}

void SetDynamicDebugLog(const std::string& location_relative, bool enable) {
  for (const auto& prefix : kRegisteredPrefixes) {
    try {
      DoSetDynamicDebugLog(std::string(prefix), location_relative, enable);
      return;
    } catch (const std::exception&) {
    }
  }

  throw std::runtime_error("Bad relative location: " + location_relative);
}

void RestrictDynamicDebugLocations(
    const std::unordered_set<std::string>& restrict_list) {
  auto ddl = GetDynamicDebugData().StartWrite();
  for (const auto& location : restrict_list) {
    if (location.empty()) continue;

    ddl->restricted_locations.insert(AbsoluteLocation{location});
  }
  ddl.Commit();
}

void InitDynamicDebugLog() {
  if (engine::current_task::GetCurrentTaskContextUnchecked())
    DoInitDynamicDebugLog();
  else
    RunInCoro(&DoInitDynamicDebugLog);
}

std::unordered_set<std::string> GetDynamicDebugLocations() {
  std::unordered_set<std::string> result;
  auto ddl = GetDynamicDebugData().Read();
  for (const auto& location : ddl->restricted_locations) {
    result.insert(FromAbsoluteLocation(location).GetUnderlying());
  }
  return result;
}

}  // namespace logging

USERVER_NAMESPACE_END

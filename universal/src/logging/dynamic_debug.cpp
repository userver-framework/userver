#include "dynamic_debug.hpp"

#include <cstring>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

LogEntryContentSet& GetAllLocations() noexcept {
  static LogEntryContentSet locations{};
  return locations;
}

[[noreturn]] void ThrowUnknownDynamicLogLocation(std::string_view location,
                                                 int line) {
  if (line != kAnyLine) {
    throw std::runtime_error(fmt::format(
        "dynamic-debug-log: no logging in '{}' at line {}", location, line));
  }

  throw std::runtime_error(
      fmt::format("dynamic-debug-log: no logging in '{}'", location));
}

}  // namespace

Level GetForceDisabledLevelPlusOne(Level level) {
  if (level == Level::kNone) {
    return Level::kTrace;
  }
  return static_cast<Level>(utils::UnderlyingValue(level) + 1);
}

bool operator<(const LogEntryContent& x, const LogEntryContent& y) noexcept {
  const auto cmp = std::strcmp(x.path, y.path);
  return cmp < 0 || (cmp == 0 && x.line < y.line);
}

bool operator==(const LogEntryContent& x, const LogEntryContent& y) noexcept {
  return x.line == y.line && std::strcmp(x.path, y.path) == 0;
}

void AddDynamicDebugLog(const std::string& location_relative, int line,
                        EntryState state) {
  utils::impl::AssertStaticRegistrationFinished();

  auto& all_locations = GetAllLocations();

  auto it_lower = all_locations.lower_bound({location_relative.c_str(), line});
  if (it_lower == all_locations.end() ||
      std::strncmp(it_lower->path, location_relative.c_str(),
                   location_relative.size()) != 0) {
    ThrowUnknownDynamicLogLocation(location_relative, line);
  }

  if (line != kAnyLine) {
    if (it_lower->line != line && it_lower->path != location_relative) {
      ThrowUnknownDynamicLogLocation(location_relative, line);
    }

    it_lower->state.store(state);
    return;
  } else {
    for (; it_lower != all_locations.end(); ++it_lower) {
      if (std::strncmp(it_lower->path, location_relative.c_str(),
                       location_relative.size()) != 0)
        break;
      it_lower->state.store(state);
    }
  }
}

void RemoveDynamicDebugLog(const std::string& location_relative, int line) {
  utils::impl::AssertStaticRegistrationFinished();
  auto& all_locations = GetAllLocations();

  auto it_lower = all_locations.lower_bound({location_relative.c_str(), line});
  const auto it_upper = all_locations.upper_bound(
      {location_relative.c_str(),
       line != kAnyLine ? line : std::numeric_limits<int>::max()});

  for (; it_lower != it_upper; ++it_lower) {
    it_lower->state.store(EntryState{});
  }
}

const LogEntryContentSet& GetDynamicDebugLocations() {
  utils::impl::AssertStaticRegistrationFinished();
  return GetAllLocations();
}

void RegisterLogLocation(LogEntryContent& location) {
  utils::impl::AssertStaticRegistrationAllowed(
      "Dynamic debug logging location");
  UASSERT(location.path);
  UASSERT(location.line);
  GetAllLocations().insert(location);
}

}  // namespace logging

USERVER_NAMESPACE_END

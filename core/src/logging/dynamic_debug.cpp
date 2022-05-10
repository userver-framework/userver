#include "dynamic_debug.hpp"

#include <cstring>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <utils/impl/static_registration.hpp>

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

namespace impl {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
StaticLogEntry::StaticLogEntry(const char* path, int line) noexcept {
  utils::impl::AssertStaticRegistrationAllowed(
      "Dynamic debug logging location");
  UASSERT(path);
  UASSERT(line);
  static_assert(sizeof(LogEntryContent) == sizeof(content));
  // static_assert(std::is_trivially_destructible_v<LogEntryContent>);
  auto* item = new (&content) LogEntryContent(path, line);
  GetAllLocations().insert(*item);
}

bool StaticLogEntry::ShouldLog() const noexcept {
  return reinterpret_cast<const LogEntryContent&>(content).should_log.load();
}

}  // namespace impl

bool operator<(const LogEntryContent& x, const LogEntryContent& y) noexcept {
  const auto cmp = std::strcmp(x.path, y.path);
  return cmp < 0 || (cmp == 0 && x.line < y.line);
}

bool operator==(const LogEntryContent& x, const LogEntryContent& y) noexcept {
  return x.line == y.line && std::strcmp(x.path, y.path) == 0;
}

void AddDynamicDebugLog(const std::string& location_relative, int line) {
  utils::impl::AssertStaticRegistrationFinished();

  auto& all_locations = GetAllLocations();

  auto it_lower = all_locations.lower_bound({location_relative.c_str(), line});
  if (it_lower == all_locations.end() || it_lower->path != location_relative) {
    ThrowUnknownDynamicLogLocation(location_relative, line);
  }

  auto it_upper = it_lower;
  if (line != kAnyLine) {
    if (it_lower->line != line) {
      ThrowUnknownDynamicLogLocation(location_relative, line);
    }

    ++it_upper;
  } else {
    it_upper = all_locations.upper_bound(
        {location_relative.c_str(),
         line != kAnyLine ? line : std::numeric_limits<int>::max()});
  }

  for (; it_lower != it_upper; ++it_lower) {
    it_lower->should_log = true;
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
    it_lower->should_log = false;
  }
}

const LogEntryContentSet& GetDynamicDebugLocations() {
  utils::impl::AssertStaticRegistrationFinished();
  return GetAllLocations();
}

}  // namespace logging

USERVER_NAMESPACE_END

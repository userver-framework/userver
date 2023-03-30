#include "dynamic_debug.hpp"

#include <cstring>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>

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
  return reinterpret_cast<const LogEntryContent&>(content).state.load() ==
         EntryState::kForceEnabled;
}

bool StaticLogEntry::ShouldNotLog(Level level) const noexcept {
  if (level >= Level::kWarning) return false;

  return reinterpret_cast<const LogEntryContent&>(content).state.load() ==
         EntryState::kForceDisabled;
}

}  // namespace impl

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

    it_lower->state = state;
    return;
  } else {
    for (; it_lower != all_locations.end(); ++it_lower) {
      if (std::strncmp(it_lower->path, location_relative.c_str(),
                       location_relative.size()) != 0)
        break;
      it_lower->state = state;
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
    it_lower->state = EntryState::kDefault;
  }
}

const LogEntryContentSet& GetDynamicDebugLocations() {
  utils::impl::AssertStaticRegistrationFinished();
  return GetAllLocations();
}

}  // namespace logging

USERVER_NAMESPACE_END

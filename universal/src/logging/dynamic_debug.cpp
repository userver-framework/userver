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

[[noreturn]] void ThrowUnknownDynamicLogLocation(std::string_view location, int line) {
    if (line != kAnyLine) {
        throw std::runtime_error(fmt::format("dynamic-debug-log: no logging in '{}' at line {}", location, line));
    }

    throw std::runtime_error(fmt::format("dynamic-debug-log: no logging in '{}'", location));
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

void SetDynamicDebugLog(const std::string& location_relative, int line, EntryState state) {
    utils::impl::AssertStaticRegistrationFinished();

    auto& all_locations = GetAllLocations();
    auto it_lower = all_locations.lower_bound({location_relative.c_str(), line});

    bool found_match = false;
    if (line != kAnyLine) {
        for (; it_lower != all_locations.end(); ++it_lower) {
            // check for exact match
            if (line != it_lower->line || it_lower->path != location_relative) {
                break;
            }
            it_lower->state.store(state);
            found_match = true;
        }
    } else {
        for (; it_lower != all_locations.end(); ++it_lower) {
            // compare prefixes
            if (std::strncmp(it_lower->path, location_relative.c_str(), location_relative.size()) != 0) {
                break;
            }
            it_lower->state.store(state);
            found_match = true;
        }
    }

    if (!found_match) {
        ThrowUnknownDynamicLogLocation(location_relative, line);
    }
}

void AddDynamicDebugLog(const std::string& location_relative, int line) {
    SetDynamicDebugLog(location_relative, line, EntryState{/*force_enabled_level=*/Level::kTrace});
}

void RemoveDynamicDebugLog(const std::string& location_relative, int line) {
    SetDynamicDebugLog(location_relative, line, EntryState{});
}

void RemoveAllDynamicDebugLog() {
    utils::impl::AssertStaticRegistrationFinished();
    auto& all_locations = GetAllLocations();
    for (auto& location : all_locations) {
        location.state.store(EntryState{});
    }
}

const LogEntryContentSet& GetDynamicDebugLocations() {
    utils::impl::AssertStaticRegistrationFinished();
    return GetAllLocations();
}

void RegisterLogLocation(LogEntryContent& location) {
    utils::impl::AssertStaticRegistrationAllowed("Dynamic debug logging location");
    UASSERT(location.path);
    UASSERT(location.line);
    GetAllLocations().insert(location);
}

}  // namespace logging

USERVER_NAMESPACE_END

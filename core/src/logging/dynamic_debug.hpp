#pragma once

#include <atomic>
#include <string_view>

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/set_hook.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

inline constexpr int kAnyLine = 0;

namespace bi = boost::intrusive;

enum class EntryState {
  kForceDisabled,
  kDefault,
  kForceEnabled,
};

using LogEntryContentHook =
    bi::set_base_hook<bi::optimize_size<true>, bi::link_mode<bi::normal_link>>;

struct LogEntryContent {
  LogEntryContent(const char* path, int line) noexcept
      : line(line), path(path) {}

  std::atomic<EntryState> state{EntryState::kDefault};
  const int line;
  const char* const path;
  LogEntryContentHook hook;
};

bool operator<(const LogEntryContent& x, const LogEntryContent& y) noexcept;
bool operator==(const LogEntryContent& x, const LogEntryContent& y) noexcept;

using LogEntryContentSet = bi::set<  //
    LogEntryContent,                 //
    bi::constant_time_size<false>,   //
    bi::member_hook<                 //
        LogEntryContent,             //
        LogEntryContentHook,         //
        &LogEntryContent::hook       //
        >                            //
    >;

void AddDynamicDebugLog(const std::string& location_relative, int line,
                        EntryState state = EntryState::kForceEnabled);

void RemoveDynamicDebugLog(const std::string& location_relative, int line);

const LogEntryContentSet& GetDynamicDebugLocations();

}  // namespace logging

USERVER_NAMESPACE_END

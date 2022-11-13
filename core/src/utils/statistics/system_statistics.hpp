#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

struct SystemStats {
  std::optional<double> cpu_time_sec;
  std::optional<std::int64_t> rss_kb;
  std::optional<std::int64_t> open_files;
  std::optional<std::int64_t> major_pagefaults;
  std::optional<std::int64_t> io_read_bytes;
  std::optional<std::int64_t> io_write_bytes;
};

void DumpMetric(Writer& writer, const SystemStats& stats);

SystemStats GetSelfSystemStatistics();
SystemStats GetSystemStatisticsByExeName(std::string_view name);

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END

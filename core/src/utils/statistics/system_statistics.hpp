#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <formats/serialize/to.hpp>

namespace formats::json {
class Value;
}  // namespace formats::json

namespace utils::statistics::impl {

struct SystemStats {
  std::optional<double> cpu_time_sec;
  std::optional<int64_t> rss_kb;
  std::optional<int64_t> open_files;
  std::optional<int64_t> major_pagefaults;
  std::optional<int64_t> io_read_bytes;
  std::optional<int64_t> io_write_bytes;
};

formats::json::Value Serialize(SystemStats stats,
                               formats::serialize::To<formats::json::Value>);

SystemStats GetSelfSystemStatistics();
SystemStats GetSystemStatisticsByExeName(std::string_view name);

}  // namespace utils::statistics::impl

#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include <formats/json/value.hpp>
#include <formats/serialize/to.hpp>

namespace dump {

struct Statistics {
  std::atomic<bool> is_loaded{false};
  std::atomic<bool> is_current_from_dump{false};
  std::atomic<std::chrono::milliseconds> load_duration{{}};

  std::atomic<std::chrono::steady_clock::time_point>
      last_nontrivial_write_start_time{{}};
  std::atomic<std::chrono::milliseconds> last_nontrivial_write_duration{{}};
  std::atomic<std::size_t> last_written_size{0};
};

formats::json::Value Serialize(const Statistics& stats,
                               formats::serialize::To<formats::json::Value>);

}  // namespace dump

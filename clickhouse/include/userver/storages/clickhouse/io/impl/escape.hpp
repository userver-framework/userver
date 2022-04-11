#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/clickhouse/io/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::impl {

std::string Escape(uint8_t);
std::string Escape(uint16_t);
std::string Escape(uint32_t);
std::string Escape(uint64_t);
std::string Escape(int8_t);
std::string Escape(int16_t);
std::string Escape(int32_t);
std::string Escape(int64_t);

std::string Escape(std::string_view source);

std::string Escape(std::chrono::system_clock::time_point source);
std::string Escape(DateTime64Milli source);
std::string Escape(DateTime64Micro source);
std::string Escape(DateTime64Nano source);

template <typename T>
std::string Escape(const std::vector<T>& source) {
  // just an approximation, wild guess
  constexpr size_t kResultReserveMultiplier = 5;

  std::string result;
  result.reserve(source.size() * kResultReserveMultiplier);
  result.push_back('[');
  for (size_t i = 0; i < source.size(); ++i) {
    if (i != 0) {
      result.push_back(',');
    }
    result += impl::Escape(source[i]);
  }
  result.push_back(']');

  return result;
}

}  // namespace storages::clickhouse::io::impl

USERVER_NAMESPACE_END

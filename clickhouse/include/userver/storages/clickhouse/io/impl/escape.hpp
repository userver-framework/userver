#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/clickhouse/io/type_traits.hpp>
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

std::string Escape(const char* source);
std::string Escape(const std::string& source);
std::string Escape(std::string_view source);

std::string Escape(std::chrono::system_clock::time_point source);
std::string Escape(DateTime64Milli source);
std::string Escape(DateTime64Micro source);
std::string Escape(DateTime64Nano source);

template <typename Container>
std::string Escape(const Container& source) {
  static_assert(traits::kIsRange<Container>,
                "There's no escape implementation for the type.");

  std::string result;
  if constexpr (traits::kIsSizeable<Container>) {
    // just an approximation, wild guess
    constexpr size_t kResultReserveMultiplier = 5;
    result.reserve(source.size() * kResultReserveMultiplier);
  }
  result.push_back('[');

  bool is_first_item = true;
  for (const auto& item : source) {
    if (!is_first_item) {
      result.push_back(',');
    }
    result += impl::Escape(item);
    is_first_item = false;
  }
  result.push_back(']');

  return result;
}

}  // namespace storages::clickhouse::io::impl

USERVER_NAMESPACE_END

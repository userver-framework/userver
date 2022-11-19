#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class HttpCodes final {
 public:
  using Code = int;
  using Counter = std::uint64_t;

  static constexpr Code kMinHttpStatus = 100;
  static constexpr Code kMaxHttpStatus = 600;
  class Snapshot;

  HttpCodes();
  HttpCodes(const HttpCodes&) = delete;
  HttpCodes& operator=(const HttpCodes&) = delete;

  void Account(Code code) noexcept;

 private:
  using ValueType = std::uint64_t;
  std::array<std::atomic<ValueType>, kMaxHttpStatus - kMinHttpStatus> codes_{};
};

class HttpCodes::Snapshot final {
 public:
  Snapshot() = default;
  Snapshot(const Snapshot&) = default;
  Snapshot& operator=(const Snapshot&) = default;

  explicit Snapshot(const HttpCodes& other) noexcept;

  void operator+=(const Snapshot& other);

  friend void DumpMetric(Writer& writer, const Snapshot& snapshot);

 private:
  std::array<HttpCodes::ValueType, kMaxHttpStatus - kMinHttpStatus> codes_{};
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END

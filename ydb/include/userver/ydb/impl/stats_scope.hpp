#pragma once

#include <chrono>
#include <string>

#include <userver/ydb/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

struct Stats;
struct StatsCounters;

class StatsScope final {
 public:
  struct TransactionTag {};

  StatsScope(Stats& stats, const Query& query);
  StatsScope(TransactionTag, Stats& stats, const std::string& tx_name);

  StatsScope(StatsScope&&) noexcept;
  ~StatsScope();

  void OnError() noexcept;
  void OnCancelled() noexcept;

 private:
  explicit StatsScope(StatsCounters&);

  StatsCounters& stats_;
  const std::chrono::steady_clock::time_point start_;
  bool is_active_{true};
  bool is_error_{false};
  bool is_cancelled_{false};
};

}  // namespace ydb::impl

USERVER_NAMESPACE_END

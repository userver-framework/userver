#pragma once

#include <memory>
#include <string>

#include <userver/utils/retry_budget.hpp>
#include <userver/utils/statistics/fwd.hpp>

namespace NMonitoring {
class TMetricRegistry;
}  // namespace NMonitoring

namespace NYdb {
class TDriver;
}

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

struct DriverSettings;

class Driver final {
 public:
  Driver(std::string dbname, impl::DriverSettings settings);

  Driver(const Driver&) = delete;
  Driver(Driver&&) noexcept = delete;
  Driver& operator=(const Driver&) = delete;
  Driver operator=(Driver&&) = delete;

  ~Driver();

  const NYdb::TDriver& GetNativeDriver() const;

  /// Nickname of the database, used by userver in configs and logs.
  const std::string& GetDbName() const;
  /// Path of the database.
  const std::string& GetDbPath() const;

  utils::RetryBudget& GetRetryBudget();

  friend void DumpMetric(utils::statistics::Writer& writer,
                         const Driver& client);

 private:
  const std::string dbname_;
  const std::string dbpath_;

  std::unique_ptr<NMonitoring::TMetricRegistry> native_metrics_;
  // The retry_budget_ is used in driver_ threads, so it must be before the
  // driver_
  utils::RetryBudget retry_budget_;
  std::unique_ptr<NYdb::TDriver> driver_;
};

std::string JoinPath(std::string_view database_path, std::string_view path);

}  // namespace ydb::impl

USERVER_NAMESPACE_END

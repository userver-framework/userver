#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <ydb-cpp-sdk/client/driver/fwd.h>
#include <ydb-cpp-sdk/client/types/executor/executor.h>

namespace NMonitoring {
class TMetricRegistry;
}  // namespace NMonitoring

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

struct DriverSettings;

class UserverExecutor final : public NYdb::IExecutor {
public:
    explicit UserverExecutor(engine::TaskProcessor& task_processor);

    void Post(TFunction&& task) override;
    bool IsAsync() const override;
    void Stop() override;

private:
    void DoStart() override;

    engine::TaskProcessor& task_processor_;
    concurrent::BackgroundTaskStorageCore tasks_;
};

class Driver final {
public:
    Driver(std::string dbname, impl::DriverSettings settings, engine::TaskProcessor& task_processor);

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

    friend void DumpMetric(utils::statistics::Writer& writer, const Driver& driver);

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

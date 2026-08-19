#pragma once

#include <chrono>
#include <optional>

#include <userver/storages/odbc/query.hpp>
#include <userver/utils/datetime.hpp>

#include <storages/odbc/detail/statement_stats_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class StatementStats final {
public:
    StatementStats(const Query& query, StatementStatsStorage& storage) noexcept;

    void AccountSuccess() noexcept;
    void AccountError() noexcept;

private:
    void Account(StatementStatsStorage::ExecutionResult result) noexcept;

    StatementStatsStorage* storage_{nullptr};
    std::optional<Query::NameView> name_;
    std::optional<StatementStatsStorage::GenerationToken> generation_;
    utils::datetime::SteadyClock::time_point start_{};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

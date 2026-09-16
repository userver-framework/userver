#include <storages/odbc/detail/statement_stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

StatementStats::StatementStats(const Query& query, StatementStatsStorage& storage) noexcept
    : storage_{&storage},
      name_{query.GetOptionalNameView()},
      generation_{storage.GetGenerationToken()}
{
    if (!name_ || !generation_) {
        storage_ = nullptr;
        return;
    }
    if (storage_) {
        start_ = utils::datetime::SteadyClock::now();
    }
}

void StatementStats::AccountSuccess() noexcept { Account(StatementStatsStorage::ExecutionResult::kSuccess); }

void StatementStats::AccountError() noexcept { Account(StatementStatsStorage::ExecutionResult::kError); }

void StatementStats::Account(StatementStatsStorage::ExecutionResult result) noexcept {
    if (!storage_ || !name_) {
        return;
    }

    const auto
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(utils::datetime::SteadyClock::now() - start_);
    storage_->Account(*name_, static_cast<std::size_t>(elapsed.count()), result, *generation_);
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

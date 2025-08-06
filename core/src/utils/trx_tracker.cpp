#include <userver/utils/trx_tracker.hpp>

#include <userver/engine/task/local_variable.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::trx_tracker {

namespace {

struct TransactionTracker final {
    std::uint32_t trx_count = 0;
    std::uint32_t disabler_count = 0;
};

struct TransactionTrackerStatisticsInternal final {
    utils::statistics::RateCounter triggers{0};
};

engine::TaskLocalVariable<TransactionTracker> transaction_tracker{};
TransactionTrackerStatisticsInternal transaction_tracker_statistics{};

}  // namespace

void StartTransaction() { ++transaction_tracker->trx_count; }

void EndTransaction() noexcept {
    UASSERT(transaction_tracker.GetOptional() != nullptr);
    --transaction_tracker->trx_count;
}

void CheckNoTransactions(utils::impl::SourceLocation location) {
    auto* tracker = transaction_tracker.GetOptional();
    if (tracker && !tracker->disabler_count && tracker->trx_count) {
        logging::LogExtra log_extra;
        log_extra.Extend("location", utils::impl::ToString(location));
        LOG_LIMITED_WARNING() << "Long call while having active transactions" << std::move(log_extra);
        ++transaction_tracker_statistics.triggers;

        TESTPOINT("long_call_in_trx", [&location] {
            formats::json::ValueBuilder builder;
            builder["source"] = utils::impl::ToString(location);
            return builder.ExtractValue();
        }());
    }
}

TransactionTrackerStatistics GetStatistics() noexcept {
    return TransactionTrackerStatistics{transaction_tracker_statistics.triggers.Load()};
}

void ResetStatistics() { utils::statistics::ResetMetric(transaction_tracker_statistics.triggers); }

CheckDisabler::CheckDisabler() { ++transaction_tracker->disabler_count; }

CheckDisabler::~CheckDisabler() { Reenable(); }

void CheckDisabler::Reenable() noexcept {
    if (!reenabled_) {
        --transaction_tracker->disabler_count;
        reenabled_ = true;
    }
}

}  // namespace utils::trx_tracker

USERVER_NAMESPACE_END

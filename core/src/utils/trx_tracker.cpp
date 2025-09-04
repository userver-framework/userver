#include <userver/utils/trx_tracker.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/function_ref.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::trx_tracker {

namespace {

compiler::ThreadLocal local_task_counter = []() -> std::uint64_t { return 0; };

struct TransactionTracker final {
    std::uint32_t trx_count = 0;
    std::uint32_t disabler_count = 0;
    impl::TaskId task_id;
};

struct TransactionTrackerStatisticsInternal final {
    utils::statistics::RateCounter triggers{0};
};

engine::TaskLocalVariable<TransactionTracker> transaction_tracker{};
TransactionTrackerStatisticsInternal transaction_tracker_statistics{};

void CheckNoTransactions(utils::function_ref<std::string()> get_location) {
    if (!impl::IsEnabled()) {
        return;
    }

    auto* tracker = transaction_tracker.GetOptional();
    if (tracker && !tracker->disabler_count && tracker->trx_count > 0) {
        const std::string location = get_location();
        logging::LogExtra log_extra;
        log_extra.Extend("location", location);
        LOG_LIMITED_WARNING() << "Long call while having active transactions" << std::move(log_extra);
        ++transaction_tracker_statistics.triggers;

        TESTPOINT("long_call_in_trx", [&location] {
            formats::json::ValueBuilder builder;
            builder["location"] = location;
            return builder.ExtractValue();
        }());
    }
}

std::optional<impl::TaskId> StartTransaction() {
    if (impl::IsEnabled()) {
        ++transaction_tracker->trx_count;
        return transaction_tracker->task_id;
    }
    return std::nullopt;
}

bool EndTransaction(const impl::TaskId& task_id) noexcept {
    if (impl::IsEnabled()) {
        if (transaction_tracker.GetOptional() == nullptr || transaction_tracker->task_id != task_id) {
            // EndTransaction is called in different task than StartTransaction
            return false;
        }
        --transaction_tracker->trx_count;
        return true;
    }
    return true;
}

bool trx_tracker_enabled = false;
bool trx_tracker_enabler_exists = false;

}  // namespace

namespace impl {

GlobalEnabler::GlobalEnabler(bool enable) {
    UASSERT(!trx_tracker_enabler_exists);
    trx_tracker_enabled = enable;
    trx_tracker_enabler_exists = true;
}

GlobalEnabler::~GlobalEnabler() {
    trx_tracker_enabled = false;
    trx_tracker_enabler_exists = false;
}

bool IsEnabled() noexcept { return trx_tracker_enabled; }

TaskId::TaskId() : created_thread_id_{std::this_thread::get_id()} {
    auto counter_scope{local_task_counter.Use()};
    thread_local_counter_ = (*counter_scope)++;
}

bool TaskId::operator==(const TaskId& other) const {
    return created_thread_id_ == other.created_thread_id_ && thread_local_counter_ == other.thread_local_counter_;
}

bool TaskId::operator!=(const TaskId& other) const { return !(*this == other); }

}  // namespace impl

TransactionLock::~TransactionLock() { Unlock(); }

void TransactionLock::Lock() noexcept {
    if (!task_id_.has_value()) {
        task_id_ = StartTransaction();
    }
}

void TransactionLock::Unlock() noexcept {
    if (task_id_.has_value() && EndTransaction(task_id_.value())) {
        task_id_ = std::nullopt;
    }
}

TransactionLock::TransactionLock(TransactionLock&& other) noexcept
    : task_id_(std::exchange(other.task_id_, std::nullopt)) {}

TransactionLock& TransactionLock::operator=(TransactionLock&& other) noexcept {
    if (&other == this) {
        return *this;
    }

    Unlock();
    task_id_ = std::exchange(other.task_id_, std::nullopt);
    return *this;
}

void CheckNoTransactions(utils::impl::SourceLocation location) {
    CheckNoTransactions([&location]() { return utils::impl::ToString(location); });
}

void CheckNoTransactions(std::string_view location) {
    CheckNoTransactions([&location]() { return std::string(location); });
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

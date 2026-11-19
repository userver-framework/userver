#include <userver/storages/postgres/dist_lock_strategy.hpp>

#include <stdexcept>
#include <string_view>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/storages/postgres/cluster.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {

// key - $1
// owner - $2
// timeout in seconds - $3
Query MakeAcquireQuery(std::string_view table) {
    static constexpr std::string_view kAcquireQueryFmt = R"(
    INSERT INTO {} AS t (key, owner, expiration_time) SELECT
    $1, $2, current_timestamp + make_interval(secs => $3)
    -- don't insert any records when actual lock from another owner is exists
    WHERE NOT EXISTS (
        -- actual lock from another owner
        SELECT *
        FROM {}
        WHERE key = $1
          AND owner <> $2
          AND expiration_time > current_timestamp
    )
    ON CONFLICT (key) DO UPDATE
    SET owner = $2, expiration_time = current_timestamp + make_interval(secs => $3)
    WHERE (t.owner = $2) OR
    (t.expiration_time <= current_timestamp) RETURNING 1;
)";
    return {fmt::format(FMT_COMPILE(kAcquireQueryFmt), table, table), Query::Name{"dist_lock_acquire"}};
}

// key - $1
// owner - $2
// timeout in seconds - $3
// minimum idle time in milliseconds - $4
Query MakeFallbackAcquireQuery(std::string_view table) {
    static constexpr std::string_view kAcquireQueryFmt = R"(
    INSERT INTO {} AS t (key, owner, expiration_time) SELECT
    $1, $2, clock_timestamp() + make_interval(secs => $3)
    WHERE NOT EXISTS (
        SELECT 1
        FROM {}
        WHERE key = $1
          AND expiration_time > clock_timestamp() - $4 * INTERVAL '1 millisecond'
    )
    ON CONFLICT (key) DO UPDATE
    SET owner = $2, expiration_time = clock_timestamp() + make_interval(secs => $3)
    WHERE t.expiration_time <= clock_timestamp() - $4 * INTERVAL '1 millisecond'
    RETURNING 1;
)";
    return {fmt::format(FMT_COMPILE(kAcquireQueryFmt), table, table), Query::Name{"dist_lock_fallback_acquire"}};
}

Query MakeCheckQuery(std::string_view table) {
    static constexpr std::string_view kCheckQueryFmt = R"(
    SELECT 1 FROM {}
    WHERE key = $1 AND expiration_time > clock_timestamp();
)";
    return {fmt::format(FMT_COMPILE(kCheckQueryFmt), table), Query::Name{"dist_lock_check"}};
}

// key - $1
// owner - $2
// timeout in seconds - $3
Query MakeProlongQuery(std::string_view table) {
    static constexpr std::string_view kProlongQueryFmt = R"(
    UPDATE {}
    SET expiration_time = current_timestamp + make_interval(secs => $3)
    WHERE key = $1
      AND owner = $2
    RETURNING 1;
)";
    return {fmt::format(FMT_COMPILE(kProlongQueryFmt), table), Query::Name{"dist_lock_prolong"}};
}

// key - $1
// owner - $2
Query MakeReleaseQuery(std::string_view table) {
    static constexpr std::string_view kReleaseQueryFmt = R"(
    DELETE FROM {} WHERE key = $1 AND owner = $2 RETURNING 1;
)";
    return {fmt::format(FMT_COMPILE(kReleaseQueryFmt), table), Query::Name{"dist_lock_release"}};
}

// key - $1
// owner - $2
Query MakeReleaseWithHistoryQuery(std::string_view table) {
    static constexpr std::string_view kReleaseQueryFmt = R"(
    UPDATE {}
    SET owner = '', expiration_time = clock_timestamp()
    WHERE key = $1 AND owner = $2 RETURNING 1;
)";
    return {fmt::format(FMT_COMPILE(kReleaseQueryFmt), table), Query::Name{"dist_lock_release_with_history"}};
}

std::string MakeOwnerId(std::string_view prefix, std::string_view locker) {
    return fmt::format(FMT_COMPILE("{}:{}"), prefix, locker);
}

}  // namespace

DistLockStrategy::DistLockStrategy(
    ClusterPtr cluster,
    std::string_view table,
    std::string_view lock_name,
    const dist_lock::DistLockSettings& settings
)
    : cluster_(std::move(cluster)),
      cc_(settings.forced_stop_margin, settings.forced_stop_margin),
      acquire_query_(MakeAcquireQuery(table)),
      fallback_acquire_query_(MakeFallbackAcquireQuery(table)),
      check_query_(MakeCheckQuery(table)),
      prolong_query_(MakeProlongQuery(table)),
      release_query_(MakeReleaseQuery(table)),
      release_with_history_query_(MakeReleaseWithHistoryQuery(table)),
      lock_name_(lock_name),
      owner_prefix_(hostinfo::blocking::GetRealHostName())
{}

void DistLockStrategy::UpdateCommandControl(CommandControl cc) {
    auto cc_ptr = cc_.StartWrite();
    *cc_ptr = cc;
    cc_ptr.Commit();
}

dist_lock::LockOperationResult DistLockStrategy::RunLockQuery(
    std::string_view locker_id,
    const dist_lock::LockOperationOptions& options
) {
    auto cc_ptr = cc_.Read();
    // make_interval(secs => ...) accepts seconds, including fractional seconds.
    const auto lock_ttl_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(options.lock_ttl).count();
    const auto owner_id = MakeOwnerId(owner_prefix_, locker_id);
    try {
        if (options.operation == dist_lock::LockOperation::kFallback) {
            const auto min_delay_milliseconds = options.min_idle_time_milliseconds.count();
            const auto result = cluster_->Execute(
                ClusterHostType::kMaster,
                *cc_ptr,
                fallback_acquire_query_,
                lock_name_,
                owner_id,
                lock_ttl_seconds,
                min_delay_milliseconds
            );
            return result.IsEmpty() ? dist_lock::LockOperationResult::kBusy : dist_lock::LockOperationResult::kAcquired;
        }
        const auto& query = options.operation == dist_lock::LockOperation::kProlong ? prolong_query_ : acquire_query_;
        const auto result =
            cluster_->Execute(ClusterHostType::kMaster, *cc_ptr, query, lock_name_, owner_id, lock_ttl_seconds);
        return result.IsEmpty() ? dist_lock::LockOperationResult::kBusy : dist_lock::LockOperationResult::kAcquired;
    } catch (const TransactionRollback& exc) {
        if (exc.GetSqlState() == SqlState::kSerializationFailure) {
            //  Looks like the default transaction isolation is 'repeatable read' or 'serializable' and we were hit by
            //  "could not serialize access due to concurrent update"
            return dist_lock::LockOperationResult::kBusy;
        } else {
            throw;
        }
    }
}

dist_lock::LockOperationResult DistLockStrategy::CheckLock() {
    const auto cc_ptr = cc_.Read();
    const auto result = cluster_->Execute(ClusterHostType::kMaster, *cc_ptr, check_query_, lock_name_);
    return result.IsEmpty() ? dist_lock::LockOperationResult::kNotAlive : dist_lock::LockOperationResult::kBusy;
}

void DistLockStrategy::Acquire(std::chrono::milliseconds lock_ttl, const std::string& locker_id) {
    if (ExecuteOperation(locker_id, {.lock_ttl = lock_ttl}) != dist_lock::LockOperationResult::kAcquired) {
        throw dist_lock::LockIsAcquiredByAnotherHostException{};
    }
}

dist_lock::LockOperationResult DistLockStrategy::ExecuteOperation(
    const std::string& locker_id,
    const dist_lock::LockOperationOptions& options
) {
    switch (options.operation) {
        case dist_lock::LockOperation::kAcquire:
        case dist_lock::LockOperation::kProlong:
            return RunLockQuery(locker_id, options);
        case dist_lock::LockOperation::kCheck:
            return CheckLock();
        case dist_lock::LockOperation::kFallback:
            if (options.min_idle_time_milliseconds < std::chrono::milliseconds::zero()) {
                throw std::invalid_argument{"The fallback minimum idle interval must not be negative"};
            }
            return RunLockQuery(locker_id, options);
    }
    return dist_lock::LockOperationResult::kUnsupported;
}

void DistLockStrategy::Prolong(std::chrono::milliseconds lock_ttl, const std::string& locker_id) {
    const dist_lock::LockOperationOptions options{
        .lock_ttl = lock_ttl,
        .operation = dist_lock::LockOperation::kProlong,
    };
    if (ExecuteOperation(locker_id, options) != dist_lock::LockOperationResult::kAcquired) {
        throw dist_lock::LockIsAcquiredByAnotherHostException{};
    }
}

void DistLockStrategy::Release(const std::string& locker_id) { Release(locker_id, {}); }

void DistLockStrategy::Release(const std::string& locker_id, const dist_lock::ReleaseOptions& options) {
    const auto cc_ptr = cc_.Read();
    const auto& query = options.preserve_history ? release_with_history_query_ : release_query_;
    const auto result =
        cluster_->Execute(ClusterHostType::kMaster, *cc_ptr, query, lock_name_, MakeOwnerId(owner_prefix_, locker_id));
    if (result.IsEmpty()) {
        throw dist_lock::LockIsAcquiredByAnotherHostException{};
    }
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END

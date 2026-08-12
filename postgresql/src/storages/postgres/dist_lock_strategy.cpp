#include <userver/storages/postgres/dist_lock_strategy.hpp>

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
    DELETE FROM {}
    WHERE key = $1
    AND owner = $2
    RETURNING 1;
)";
    return {fmt::format(FMT_COMPILE(kReleaseQueryFmt), table), Query::Name{"dist_lock_release"}};
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
      prolong_query_(MakeProlongQuery(table)),
      release_query_(MakeReleaseQuery(table)),
      lock_name_(lock_name),
      owner_prefix_(hostinfo::blocking::GetRealHostName())
{}

void DistLockStrategy::UpdateCommandControl(CommandControl cc) {
    auto cc_ptr = cc_.StartWrite();
    *cc_ptr = cc;
    cc_ptr.Commit();
}

void DistLockStrategy::RunLockQuery(
    const Query& query,
    std::chrono::milliseconds lock_ttl,
    std::string_view locker_id
) {
    auto cc_ptr = cc_.Read();
    const auto timeout_seconds = std::chrono::duration<double>{lock_ttl}.count();

    try {
        auto result = cluster_->Execute(
            ClusterHostType::kMaster,
            *cc_ptr,
            query,
            lock_name_,
            MakeOwnerId(owner_prefix_, locker_id),
            timeout_seconds
        );

        if (result.IsEmpty()) {
            throw dist_lock::LockIsAcquiredByAnotherHostException();
        }
    } catch (const TransactionRollback& exc) {
        if (exc.GetSqlState() == SqlState::kSerializationFailure) {
            //  Looks like the default transaction isolation is 'repeatable read' or 'serializable' and we were hit by
            //  "could not serialize access due to concurrent update"
            throw dist_lock::LockIsAcquiredByAnotherHostException();
        } else {
            throw;
        }
    }
}

void DistLockStrategy::Acquire(std::chrono::milliseconds lock_ttl, const std::string& locker_id) {
    RunLockQuery(acquire_query_, lock_ttl, locker_id);
}

void DistLockStrategy::Prolong(std::chrono::milliseconds lock_ttl, const std::string& locker_id) {
    RunLockQuery(prolong_query_, lock_ttl, locker_id);
}

void DistLockStrategy::Release(const std::string& locker_id) {
    auto cc_ptr = cc_.Read();
    cluster_
        ->Execute(ClusterHostType::kMaster, *cc_ptr, release_query_, lock_name_, MakeOwnerId(owner_prefix_, locker_id));
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END

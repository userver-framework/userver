#include <userver/storages/postgres/dist_lock_strategy.hpp>

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
std::string MakeAcquireQuery(const std::string& table) {
  static constexpr auto kAcquireQueryFmt = R"(
    INSERT INTO {} AS t (key, owner, expiration_time) VALUES
    ($1, $2, current_timestamp + make_interval(secs => $3))
    ON CONFLICT (key) DO UPDATE
    SET owner = $2, expiration_time = current_timestamp + make_interval(secs => $3)
    WHERE (t.owner = $2) OR
    (t.expiration_time <= current_timestamp) RETURNING 1;
)";
  return fmt::format(FMT_COMPILE(kAcquireQueryFmt), table);
}

// key - $1
// owner - $2
std::string MakeReleaseQuery(const std::string& table) {
  static constexpr auto kReleaseQueryFmt = R"(
    DELETE FROM {}
    WHERE key = $1
    AND owner = $2
    RETURNING 1;
)";
  return fmt::format(FMT_COMPILE(kReleaseQueryFmt), table);
}

std::string MakeOwnerId(const std::string& prefix, const std::string& locker) {
  return fmt::format(FMT_COMPILE("{}:{}"), prefix, locker);
}

}  // namespace

DistLockStrategy::DistLockStrategy(ClusterPtr cluster, const std::string& table,
                                   const std::string& lock_name,
                                   const dist_lock::DistLockSettings& settings)
    : cluster_(std::move(cluster)),
      cc_(settings.forced_stop_margin, settings.forced_stop_margin),
      acquire_query_(MakeAcquireQuery(table)),
      release_query_(MakeReleaseQuery(table)),
      lock_name_(lock_name),
      owner_prefix_(hostinfo::blocking::GetRealHostName()) {}

void DistLockStrategy::UpdateCommandControl(CommandControl cc) {
  auto cc_ptr = cc_.StartWrite();
  *cc_ptr = cc;
  cc_ptr.Commit();
}

void DistLockStrategy::Acquire(std::chrono::milliseconds lock_ttl,
                               const std::string& locker_id) {
  double timeout_seconds = lock_ttl.count() / 1000.0;
  auto cc_ptr = cc_.Read();
  auto result = cluster_->Execute(
      ClusterHostType::kMaster, *cc_ptr, acquire_query_, lock_name_,
      MakeOwnerId(owner_prefix_, locker_id), timeout_seconds);

  if (result.IsEmpty()) throw dist_lock::LockIsAcquiredByAnotherHostException();
}

void DistLockStrategy::Release(const std::string& locker_id) {
  auto cc_ptr = cc_.Read();
  cluster_->Execute(ClusterHostType::kMaster, *cc_ptr, release_query_,
                    lock_name_, MakeOwnerId(owner_prefix_, locker_id));
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END

#include <storages/postgres/dist_locked_task.hpp>

#include <storages/postgres/cluster.hpp>

#include <fmt/format.h>
#include <blocking/system/get_hostname.hpp>

namespace storages {
namespace postgres {

namespace {

// key - $1
// owner - $2
// timeout in seconds - $3
std::string MakeAcquireQuery(const std::string& table) {
  static const auto kAcquireQueryFmt = R"(
    INSERT INTO {} AS t (key, owner, expiration_time) VALUES
    ($1, $2, current_timestamp + make_interval(secs => $3))
    ON CONFLICT (key) DO UPDATE
    SET owner = $2, expiration_time = current_timestamp + make_interval(secs => $3)
    WHERE (t.owner = $2) OR
    (t.expiration_time <= current_timestamp) RETURNING 1;
)";
  return fmt::format(kAcquireQueryFmt, table);
}

// key - $1
// owner - $2
std::string MakeReleaseQuery(const std::string& table) {
  static const auto kReleaseQueryFmt = R"(
    DELETE FROM {}
    WHERE key = $1
    AND owner = $2
    RETURNING 1;
)";
  return fmt::format(kReleaseQueryFmt, table);
}

}  // namespace

DistLockedTask::DistLockedTask(ClusterPtr cluster, const std::string& table,
                               const std::string& lock_name,
                               WorkerFunc worker_func,
                               const DistLockedTaskSettings& settings,
                               Mode mode)
    : storages::DistLockedTask(lock_name, std::move(worker_func), settings,
                               mode),
      cluster_(std::move(cluster)),
      cc_(std::make_unique<CommandControl>(settings.prolong_critical_interval,
                                           settings.prolong_critical_interval)),
      acquire_query_(MakeAcquireQuery(table)),
      release_query_(MakeReleaseQuery(table)),
      lock_name_(lock_name),
      owner_(blocking::system::GetRealHostName()) {}

void DistLockedTask::UpdateCommandControl(CommandControl cc) {
  auto cc_ptr = cc_.StartWrite();
  *cc_ptr = cc;
  cc_ptr.Commit();
}

void DistLockedTask::RequestAcquire(std::chrono::milliseconds lock_time) {
  double timeout_seconds = lock_time.count() / 1000.0;
  auto result =
      cluster_->Execute(ClusterHostType::kMaster, *cc_.Read(), acquire_query_,
                        lock_name_, owner_, timeout_seconds);

  if (result.IsEmpty()) throw LockIsAcquiredByAnotherHostError();
}

void DistLockedTask::RequestRelease() {
  cluster_->Execute(ClusterHostType::kMaster, *cc_.Read(), release_query_,
                    lock_name_, owner_);
}

}  // namespace postgres
}  // namespace storages

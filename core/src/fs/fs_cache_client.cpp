#include <userver/fs/fs_cache_client.hpp>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

FsCacheClient::FsCacheClient(std::string_view dir,
                             std::chrono::milliseconds update_period,
                             engine::TaskProcessor& tp)
    : dir_(dir), update_period_(update_period), tp_(tp) {
  UpdateCache();

  if (update_period_ == std::chrono::milliseconds(0)) {
    return;
  }

  cache_updater_.Start("fs_cache_updater",
                       utils::PeriodicTask::Settings{update_period_},
                       [this] { UpdateCache(); });
}

void FsCacheClient::UpdateCache() {
  auto map = fs::ReadRecursiveFilesInfoWithData(
      tp_, dir_, {fs::SettingsReadFile::kSkipHidden});
  data_.Assign(std::move(map));
}

FileInfoWithDataConstPtr FsCacheClient::TryGetFile(
    std::string_view path) const {
  LOG_DEBUG() << "Find file " << path;
  auto snap = data_.GetSnapshot();
  if (auto it = snap.find(std::string{path}); it != snap.end())
    return it->second;
  return nullptr;
}

}  // namespace fs

USERVER_NAMESPACE_END

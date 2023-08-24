#pragma once

/// @file userver/fs/fs_cache_client.hpp
/// @brief @copybref fs::FsCacheClient

#include <userver/fs/read.hpp>
#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

/// @ingroup userver_clients
///
/// @brief Class client for storing files in memory
/// Usually retrieved from `components::FsCache`
class FsCacheClient final {
 public:
  /// @brief Fills the cache and starts periodic update
  /// @param dir directory to cache files from
  /// @param update_period time (0 - fill the cache only at startup)
  /// @param tp task processor to do filesystem operations
  FsCacheClient(std::string_view dir, std::chrono::milliseconds update_period,
                engine::TaskProcessor& tp);

  /// @brief get file from memory
  /// @param path to file
  /// @return file info and content ; `nullptr` if no file with specified name
  /// on FS
  FileInfoWithDataConstPtr TryGetFile(std::string_view path) const;

  /// @brief Concurrency-safe cache update
  void UpdateCache();

 private:
  const std::string dir_;
  const std::chrono::milliseconds update_period_;
  engine::TaskProcessor& tp_;
  utils::PeriodicTask cache_updater_;
  rcu::RcuMap<std::string, const fs::FileInfoWithData> data_;
};

}  // namespace fs

USERVER_NAMESPACE_END

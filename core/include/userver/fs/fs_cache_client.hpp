#pragma once

/// @file userver/fs/fs_cache_client.hpp
/// @brief @copybrief fs::FsCacheClient

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

#include <userver/engine/io/sys_linux/inotify.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/fs/read.hpp>
#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

namespace impl {

struct FsCacheRcuMapTraits : public rcu::DefaultRcuMapTraits<std::string> {
    using Hash = utils::impl::TransparentHash<std::string>;
    using KeyEqual = std::equal_to<>;
};

}  // namespace impl

/// Default maximum file size stored in memory by FsCacheClient (1 GiB).
inline constexpr std::size_t kDefaultMaxSizeToCache = 1024ULL * 1024 * 1024;

/// @ingroup userver_clients
///
/// @brief Class client for storing files in memory
/// Usually retrieved from `components::FsCache`
class FsCacheClient final {
public:
    /// Configuration parameters for FsCacheClient.
    struct Settings final {
        /// Directory to cache files from
        std::string dir;

        /// Update period (0 - fill the cache only at startup), not used in Linux
        std::chrono::milliseconds update_period{0};

        /// Task processor to do filesystem operations
        engine::TaskProcessor& task_processor;

        /// For files larger than this limit, the full path is stored instead of the contents
        std::size_t max_size_to_cache = kDefaultMaxSizeToCache;
    };

    /// @brief Fills the cache and starts periodic update
    explicit FsCacheClient(const Settings& settings);

    /// @brief get file from memory
    /// @param path to file
    /// @return file info and content ; `nullptr` if no file with specified name
    /// on FS
    FileInfoWithDataConstPtr TryGetFile(std::string_view path) const;

    /// @brief Concurrency-safe cache update
    void UpdateCache();

private:
#ifdef __linux__
    void InotifyWork();

    void HandleDelete(const std::string& path);

    static void HandleDeleteDirectory(engine::io::sys_linux::Inotify& inotify, const std::string& path);

    void HandleCreate(const std::string& path);

    void HandleCreateDirectory(engine::io::sys_linux::Inotify& inotify, const std::string& path);

    void HandleCreateDirectoryBlocking(engine::io::sys_linux::Inotify& inotify, const std::string& path);
#endif

    const std::string dir_;
    const std::chrono::milliseconds update_period_;
    const std::size_t max_size_to_cache_;
    engine::TaskProcessor& tp_;
#ifndef __linux__
    utils::PeriodicTask cache_updater_;
#endif
    rcu::RcuMap<std::string, const fs::FileInfoWithData, impl::FsCacheRcuMapTraits> data_;

#ifdef __linux__
    engine::Task inotify_task_;
#endif
};

}  // namespace fs

USERVER_NAMESPACE_END

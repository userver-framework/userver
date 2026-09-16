#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>

#include <userver/storages/odbc/settings.hpp>

#include <storages/odbc/detail/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class PreparedStatementCacheState final {
public:
    struct SettingsSnapshot final {
        std::size_t max_size{0};
        std::size_t reset_generation{0};
    };

    explicit PreparedStatementCacheState(const settings::PreparedStatementCacheSettings& settings = {});

    SettingsSnapshot GetSettings() const noexcept;
    void SetSettings(const settings::PreparedStatementCacheSettings& settings);

    PreparedStatementCacheStatistics& GetStatistics() noexcept { return statistics_; }
    const PreparedStatementCacheStatistics& GetStatistics() const noexcept { return statistics_; }

private:
    mutable std::mutex writer_mutex_;
    std::atomic<std::size_t> version_{0};
    std::atomic<std::size_t> max_size_{0};
    std::atomic<std::size_t> reset_generation_{0};
    PreparedStatementCacheStatistics statistics_{};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

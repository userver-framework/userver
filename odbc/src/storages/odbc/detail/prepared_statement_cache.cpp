#include <storages/odbc/detail/prepared_statement_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

PreparedStatementCacheState::PreparedStatementCacheState(const settings::PreparedStatementCacheSettings& settings)
    : max_size_{settings.max_size}
{}

PreparedStatementCacheState::SettingsSnapshot PreparedStatementCacheState::GetSettings() const noexcept {
    while (true) {
        const auto version_before = version_.load(std::memory_order_acquire);
        if (version_before % 2 != 0) {
            continue;
        }
        const SettingsSnapshot result{
            .max_size = max_size_.load(std::memory_order_relaxed),
            .reset_generation = reset_generation_.load(std::memory_order_relaxed),
        };
        if (version_before == version_.load(std::memory_order_acquire)) {
            return result;
        }
    }
}

void PreparedStatementCacheState::SetSettings(const settings::PreparedStatementCacheSettings& settings) {
    const std::lock_guard lock{writer_mutex_};
    const auto old_max_size = max_size_.load(std::memory_order_relaxed);
    if (old_max_size == settings.max_size) {
        return;
    }

    version_.fetch_add(1, std::memory_order_acq_rel);
    if ((old_max_size == 0) != (settings.max_size == 0)) {
        reset_generation_.fetch_add(1, std::memory_order_relaxed);
    }
    max_size_.store(settings.max_size, std::memory_order_relaxed);
    version_.fetch_add(1, std::memory_order_release);
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

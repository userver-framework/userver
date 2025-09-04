#include <userver/storages/sqlite/options.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::settings {

ConnectionSettings ConnectionSettings::Create(const components::ComponentConfig& config) {
    ConnectionSettings settings{};
    settings.prepared_statements = config["persistent-prepared-statements"].As<bool>(kDefaultPrepareStatement)
                                       ? storages::sqlite::settings::ConnectionSettings::kCachePreparedStatements
                                       : storages::sqlite::settings::ConnectionSettings::kNoPreparedStatements;
    settings.max_prepared_cache_size = config["max_prepared_cache_size"].As<std::size_t>(kDefaultMaxPreparedCacheSize);
    return settings;
}

PoolSettings PoolSettings::Create(const components::ComponentConfig& config) {
    PoolSettings settings{};
    settings.initial_pool_size = config["initial_read_only_pool_size"].As<std::size_t>(settings.initial_pool_size);
    settings.max_pool_size = config["max_read_only_pool_size"].As<std::size_t>(settings.max_pool_size);

    UINVARIANT(
        settings.max_pool_size >= settings.initial_pool_size,
        "max_read_only_pool_size should be >= "
        "initial_read_only_pool_size, recheck your config"
    );

    return settings;
}

std::string IsolationLevelToString(const TransactionOptions::IsolationLevel& lvl) {
    switch (lvl) {
        case TransactionOptions::IsolationLevel::kSerializable:
            return "Serializable";
        case TransactionOptions::IsolationLevel::kReadUncommitted:
            return "ReadUncommitted";
        default:
            return "Unknown";
    }
}

// helper function for pretty print in tests
std::string JournalModeToString(const SQLiteSettings::JournalMode& mode) {
    switch (mode) {
        case SQLiteSettings::JournalMode::kDelete:
            return "DELETE";
        case SQLiteSettings::JournalMode::kTruncate:
            return "TRUNCATE";
        case SQLiteSettings::JournalMode::kPersist:
            return "PERSIST";
        case SQLiteSettings::JournalMode::kMemory:
            return "MEMORY";
        case SQLiteSettings::JournalMode::kWal:
            return "WAL";
        case SQLiteSettings::JournalMode::kOff:
            return "OFF";
        default:
            return "Unknown";
    }
}

}  // namespace storages::sqlite::settings

USERVER_NAMESPACE_END

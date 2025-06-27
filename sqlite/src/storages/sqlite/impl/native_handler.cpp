#include <userver/storages/sqlite/impl/native_handler.hpp>

#include <string_view>

#include <sqlite3.h>

#include <userver/engine/async.hpp>

#include <userver/logging/log.hpp>
#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

namespace {

constexpr utils::StringLiteral kPragmaJournalModeDelete = "PRAGMA journal_mode = DELETE";
constexpr utils::StringLiteral kPragmaJournalModeTruncate = "PRAGMA journal_mode = TRUNCATE";
constexpr utils::StringLiteral kPragmaJournalModePersist = "PRAGMA journal_mode = PERSIST";
constexpr utils::StringLiteral kPragmaJournalModeMemory = "PRAGMA journal_mode = MEMORY";
constexpr utils::StringLiteral kPragmaJournalModeWal = "PRAGMA journal_mode = WAL";
constexpr utils::StringLiteral kPragmaJournalModeOff = "PRAGMA journal_mode = OFF";
constexpr utils::StringLiteral kPragmaSynchronousExtra = "PRAGMA synchronous = EXTRA";
constexpr utils::StringLiteral kPragmaSynchronousFull = "PRAGMA synchronous = FULL";
constexpr utils::StringLiteral kPragmaSynchronousNormal = "PRAGMA synchronous = NORMAL";
constexpr utils::StringLiteral kPragmaSynchronousOff = "PRAGMA synchronous = OFF";
constexpr utils::StringLiteral kPragmaCacheSize = "PRAGMA cache_size = ";
constexpr utils::StringLiteral kPragmaForeignKeys = "PRAGMA foreign_keys = ";
constexpr utils::StringLiteral kPragmaJournalSizeLimit = "PRAGMA journal_size_limit = ";
constexpr utils::StringLiteral kPragmaMmapSize = "PRAGMA mmap_size = ";
constexpr utils::StringLiteral kPragmaPageSize = "PRAGMA page_size = ";
constexpr utils::StringLiteral kPragmaTempStoreFile = "PRAGMA temp_store = FILE";
constexpr utils::StringLiteral kPragmaTempStoreMemory = "PRAGMA temp_store = MEMORY";
constexpr utils::StringLiteral kPragmaReadUncommitted = "PRAGMA read_uncommitted=1";

}  // namespace

void NativeHandler::SetSettings(const settings::SQLiteSettings& settings) {
    // Set global and blocking settings in exclusive read_write connection
    if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadWrite) {
        switch (settings.journal_mode) {
            case settings::SQLiteSettings::JournalMode::kDelete:
                Exec(kPragmaJournalModeDelete);
                break;
            case settings::SQLiteSettings::JournalMode::kTruncate:
                Exec(kPragmaJournalModeTruncate);
                break;
            case settings::SQLiteSettings::JournalMode::kPersist:
                Exec(kPragmaJournalModePersist);
                break;
            case settings::SQLiteSettings::JournalMode::kMemory:
                Exec(kPragmaJournalModeMemory);
                break;
            case settings::SQLiteSettings::JournalMode::kWal:
                Exec(kPragmaJournalModeWal);
                break;
            case settings::SQLiteSettings::JournalMode::kOff:
                Exec(kPragmaJournalModeOff);
                break;
        }
        // It's settings is local for connection, but it most actual in write case
        switch (settings.synchronous) {
            case settings::SQLiteSettings::Synchronous::kExtra:
                Exec(kPragmaSynchronousExtra);
                break;
            case settings::SQLiteSettings::Synchronous::kFull:
                Exec(kPragmaSynchronousFull);
                break;
            case settings::SQLiteSettings::Synchronous::kNormal:
                Exec(kPragmaSynchronousNormal);
                break;
            case settings::SQLiteSettings::Synchronous::kOff:
                Exec(kPragmaSynchronousOff);
                break;
        }
        // It' settings work only on new database file on start
        Exec(std::string(kPragmaPageSize) + std::to_string(settings.page_size));
    }

    // Set local settings
    switch (settings.temp_store) {
        case settings::SQLiteSettings::TempStore::kFile:
            Exec(kPragmaTempStoreFile);
            break;
        case settings::SQLiteSettings::TempStore::kMemory:
            Exec(kPragmaTempStoreMemory);
            break;
    }
    if (settings.read_uncommitted) {
        Exec(kPragmaReadUncommitted);
    }
    sqlite3_busy_timeout(db_handler_, settings.busy_timeout);
    Exec(std::string(kPragmaCacheSize) + std::to_string(settings.cache_size));
    Exec(std::string(kPragmaForeignKeys) + std::to_string(settings.foreign_keys));
    Exec(std::string(kPragmaJournalSizeLimit) + std::to_string(settings.journal_size_limit));
    Exec(std::string(kPragmaMmapSize) + std::to_string(settings.mmap_size));
}

struct sqlite3* NativeHandler::OpenDatabase(const settings::SQLiteSettings& settings) {
    int flags = 0;
    if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadOnly) {
        flags |= SQLITE_OPEN_READONLY;
    } else {
        flags |= SQLITE_OPEN_READWRITE;
    }
    if (settings.create_file && settings.read_mode == settings::SQLiteSettings::ReadMode::kReadWrite) {
        flags |= SQLITE_OPEN_CREATE;
    }
    if (settings.shared_cache) {
        flags |= SQLITE_OPEN_SHAREDCACHE;
    }
    return engine::AsyncNoSpan(
               blocking_task_processor_,
               [&settings, flags] {
                   struct sqlite3* handler = nullptr;
                   if (const int ret_code = sqlite3_open_v2(settings.db_path.c_str(), &handler, flags, nullptr);
                       ret_code != SQLITE_OK) {
                       LOG_WARNING() << "Failed to open database: " << settings.db_path;
                       sqlite3_close(handler);
                       throw SQLiteException(sqlite3_errstr(ret_code), ret_code);
                   }
                   return handler;
               }
    ).Get();
}

NativeHandler::NativeHandler(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor)
    : blocking_task_processor_{blocking_task_processor}, db_handler_{OpenDatabase(settings)} {
    SetSettings(settings);
}

NativeHandler::~NativeHandler() {
    // Close all associated stmt If there are such
    sqlite3_stmt* stmt = nullptr;
    while ((stmt = sqlite3_next_stmt(db_handler_, stmt)) != nullptr) {
        sqlite3_finalize(stmt);
    }
    // Close connection (blocking I/O)
    engine::AsyncNoSpan(blocking_task_processor_, [this] { sqlite3_close(db_handler_); }).Get();
}

struct sqlite3* NativeHandler::GetHandle() const noexcept { return db_handler_; }

void NativeHandler::Exec(utils::zstring_view query) const {
    engine::AsyncNoSpan(blocking_task_processor_, [this, query] {
        if (const int ret_code = sqlite3_exec(db_handler_, query.c_str(), nullptr, nullptr, nullptr);
            ret_code != SQLITE_OK) {
            throw SQLiteException(sqlite3_errstr(ret_code), ret_code);
        }
    }).Get();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END

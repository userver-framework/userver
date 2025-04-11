#include <userver/storages/sqlite/impl/native_handler.hpp>

#include <string_view>

#include <sqlite3.h>

#include <userver/engine/async.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include "userver/logging/log.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

namespace {

constexpr std::string_view kPragmaJournalModeDelete = "PRAGMA journal_mode = DELETE";
constexpr std::string_view kPragmaJournalModeTruncate = "PRAGMA journal_mode = TRUNCATE";
constexpr std::string_view kPragmaJournalModePersist = "PRAGMA journal_mode = PERSIST";
constexpr std::string_view kPragmaJournalModeMemory = "PRAGMA journal_mode = MEMORY";
constexpr std::string_view kPragmaJournalModeWal = "PRAGMA journal_mode = WAL";
constexpr std::string_view kPragmaJournalModeOff = "PRAGMA journal_mode = OFF";
constexpr std::string_view kPragmaSynchronousExtra = "PRAGMA synchronous = EXTRA";
constexpr std::string_view kPragmaSynchronousFull = "PRAGMA synchronous = FULL";
constexpr std::string_view kPragmaSynchronousNormal = "PRAGMA synchronous = NORMAL";
constexpr std::string_view kPragmaSynchronousOff = "PRAGMA synchronous = OFF";
constexpr std::string_view kPragmaCacheSize = "PRAGMA cache_size = ";
constexpr std::string_view kPragmaForeignKeys = "PRAGMA foreign_keys = ";
constexpr std::string_view kPragmaJournalSizeLimit = "PRAGMA journal_size_limit = ";
constexpr std::string_view kPragmaMmapSize = "PRAGMA mmap_size = ";
constexpr std::string_view kPragmaPageSize = "PRAGMA page_size = ";
constexpr std::string_view kPragmaTempStoreFile = "PRAGMA temp_store = FILE";
constexpr std::string_view kPragmaTempStoreMemory = "PRAGMA temp_store = MEMORY";
constexpr std::string_view kPragmaReadUncommited = "PRAGMA read_uncommitted=1";

}  // namespace

void NativeHandler::SetSettings(const settings::SQLiteSettings& settings) {
    // Set global and blocking settings in exclusive read_write connection
    if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadWrite) {
        switch (settings.journal_mode) {
            case settings::SQLiteSettings::JournalMode::kDelete:
                Exec(kPragmaJournalModeDelete.data());
                break;
            case settings::SQLiteSettings::JournalMode::kTruncate:
                Exec(kPragmaJournalModeTruncate.data());
                break;
            case settings::SQLiteSettings::JournalMode::kPersist:
                Exec(kPragmaJournalModePersist.data());
                break;
            case settings::SQLiteSettings::JournalMode::kMemory:
                Exec(kPragmaJournalModeMemory.data());
                break;
            case settings::SQLiteSettings::JournalMode::kWal:
                Exec(kPragmaJournalModeWal.data());
                break;
            case settings::SQLiteSettings::JournalMode::kOff:
                Exec(kPragmaJournalModeOff.data());
                break;
        }
        // It's settings is local for connection, but it most actual in write case
        switch (settings.synchronous) {
            case settings::SQLiteSettings::Synchronous::kExtra:
                Exec(kPragmaSynchronousExtra.data());
                break;
            case settings::SQLiteSettings::Synchronous::kFull:
                Exec(kPragmaSynchronousFull.data());
                break;
            case settings::SQLiteSettings::Synchronous::kNormal:
                Exec(kPragmaSynchronousNormal.data());
                break;
            case settings::SQLiteSettings::Synchronous::kOff:
                Exec(kPragmaSynchronousOff.data());
                break;
        }
        // It' settings work only on new database file on start
        Exec(std::string(kPragmaPageSize) + std::to_string(settings.page_size));
    }

    // Set local settings
    switch (settings.temp_store) {
        case settings::SQLiteSettings::TempStore::kFile:
            Exec(kPragmaTempStoreFile.data());
            break;
        case settings::SQLiteSettings::TempStore::kMemory:
            Exec(kPragmaTempStoreMemory.data());
            break;
    }
    if (settings.read_uncommited) {
        Exec(kPragmaReadUncommited.data());
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
    if (settings.shared_cashe) {
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

void NativeHandler::Exec(const std::string& query) const {
    engine::AsyncNoSpan(blocking_task_processor_, [this, &query] {
        if (const int ret_code = sqlite3_exec(db_handler_, query.data(), nullptr, nullptr, nullptr);
            ret_code != SQLITE_OK) {
            throw SQLiteException(sqlite3_errstr(ret_code), ret_code);
        }
    }).Get();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END

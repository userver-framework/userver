#include <userver/storages/sqlite/impl/native_handler.hpp>

#include <userver/engine/async.hpp>

#include <userver/storages/sqlite/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

struct sqlite3* NativeHandler::OpenDatabase(
    const settings::SQLiteSettings& settings) {
  int flags = 0;
  if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadOnly) {
    flags |= SQLITE_OPEN_READONLY;
  } else {
    flags |= SQLITE_OPEN_READWRITE;
  }
  if (settings.create_file &&
      settings.read_mode == settings::SQLiteSettings::ReadMode::kReadWrite) {
    flags |= SQLITE_OPEN_CREATE;
  }
  if (settings.shared_cashe) {
    flags |= SQLITE_OPEN_SHAREDCACHE;
  }
  struct sqlite3* handler = nullptr;
  return engine::AsyncNoSpan(
             blocking_task_processor_,
             [&settings, &flags, &handler] {
               if (const int ret = sqlite3_open_v2(settings.db_name.c_str(),
                                                   &handler, flags, nullptr);
                   ret != SQLITE_OK) {
                 sqlite3_close(handler);
                 throw SQLiteException(sqlite3_errmsg(handler), ret,
                                       sqlite3_extended_errcode(handler));
               }
               sqlite3_wal_checkpoint(handler, nullptr);
               return handler;
             })
      .Get();
}

NativeHandler::NativeHandler(const settings::SQLiteSettings& settings,
                             engine::TaskProcessor& blocking_task_processor)
    : blocking_task_processor_{blocking_task_processor},
      db_handler_{OpenDatabase(settings)} {}

struct sqlite3* NativeHandler::GetHandle() const noexcept {
  return db_handler_;
}

NativeHandler::~NativeHandler() {
  engine::AsyncNoSpan(blocking_task_processor_, [this] {
    sqlite3_close(db_handler_);
  }).Get();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END

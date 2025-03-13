#pragma once

#include <userver/storages/sqlite/impl/sqlite3_include.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class NativeHandler final {
 public:
  explicit NativeHandler(const settings::SQLiteSettings& settings);

  struct sqlite3* GetHandle() const noexcept;

 private:
  struct SQLiteHandlerDeleter {
    void operator()(struct sqlite3* sqlite_handle);
  };

  using NativeHandlerPtr =
      std::unique_ptr<struct sqlite3, SQLiteHandlerDeleter>;

  NativeHandlerPtr db_handler_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END

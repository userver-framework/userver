#pragma once

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/zstring_view.hpp>

#include <userver/storages/sqlite/impl/sqlite3_include.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class NativeHandler final {
public:
    explicit NativeHandler(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor);

    ~NativeHandler();

    struct sqlite3* GetHandle() const noexcept;
    void Exec(utils::zstring_view query) const;

private:
    struct sqlite3* OpenDatabase(const settings::SQLiteSettings& settings);
    void SetSettings(const settings::SQLiteSettings& settings);

    engine::TaskProcessor& blocking_task_processor_;
    struct sqlite3* db_handler_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END

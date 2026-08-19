#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <sql.h>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>

#include <storages/odbc/detail/result_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Connection;

namespace detail {

enum class CursorTerminalResult { kSuccess, kError, kTimeout };

struct CursorStatement final {
    SQLHSTMT handle{SQL_NULL_HSTMT};
    std::vector<ResultWrapper::Column> columns;
    std::shared_ptr<void> parameter_buffers;
    std::string cache_key;
    std::size_t cache_reset_generation{0};
    std::atomic<std::size_t> fetched_so_far{0};
    std::chrono::microseconds busy_time{0};
    std::function<void(CursorTerminalResult, std::chrono::microseconds)> on_terminal;
    bool prepared{false};
    bool cacheable{false};
    bool was_cached{false};
    bool bulk_dml_validated{false};
    bool cache_allowed{false};
    bool in_transaction{false};
    bool execution_started{false};
    bool closed{false};
    bool fetching{false};
    bool finalizing{false};
    std::atomic<bool> terminal{false};
};

class CursorControl final {
public:
    explicit CursorControl(Connection& connection) noexcept : connection_{&connection} {}

private:
    friend class CursorImpl;
    friend class storages::odbc::Connection;

    mutable engine::Mutex mutex_;
    engine::ConditionVariable finalized_cv_;
    Connection* connection_;
    std::weak_ptr<CursorStatement> active_;
    std::chrono::microseconds transaction_busy_time_{0};
};

struct CursorLease final {
    std::shared_ptr<CursorControl> control;
    std::shared_ptr<CursorStatement> statement;
};

}  // namespace detail

}  // namespace storages::odbc

USERVER_NAMESPACE_END

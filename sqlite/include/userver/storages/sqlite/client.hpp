#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <exception>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/sqlite/cursor_result_set.hpp>
#include <userver/storages/sqlite/impl/binder_help.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>
#include <userver/storages/sqlite/operation_types.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/savepoint.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>
#include <userver/storages/sqlite/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @ingroup userver_clients
///
/// @brief Client interface for a SQLite connection.
/// Usually retrieved from components::SQLite
class Client final {
public:
    /// @brief Client constructor
    Client(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor);
    /// @brief Client destructor
    ~Client();

    template <typename... Args>
    ResultSet Execute(OperationType operation_type, const Query& query, const Args&... args) const;

    template <typename T>
    ResultSet ExecuteDecompose(OperationType operation_type, const Query& query, const T& row) const;

    // like
    // https://docs.python.org/3/library/sqlite3.html#sqlite3.Cursor.executemany
    template <typename Container>
    void ExecuteMany(OperationType operation_type, const Query& query, const Container& params) const;

    Transaction Begin(OperationType operation_type, const settings::TransactionOptions&) const;

    Savepoint Save(OperationType op_type, std::string name) const;

    template <typename T, typename... Args>
    CursorResultSet<T>
    GetCursor(OperationType operation_type, std::size_t batch_size, const Query& query, const Args&... args) const;

    /// Write client statistics
    void WriteStatistics(utils::statistics::Writer& writer) const;

private:
    ResultSet DoExecute(impl::io::ParamsBinderBase& params, std::shared_ptr<infra::ConnectionPtr> connection) const;

    std::shared_ptr<infra::ConnectionPtr> GetConnection(OperationType operation_type) const;

    void AccountQueryExecute(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept;
    void AccountQueryFailed(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept;

    impl::ClientImplPtr pimpl_;
};

template <typename... Args>
ResultSet Client::Execute(OperationType operation_type, const Query& query, const Args&... args) const {
    auto connection = GetConnection(operation_type);
    AccountQueryExecute(connection);
    try {
        auto params_binder = impl::BindHelper::UpdateParamsBindings(query.GetStatement(), *connection, args...);
        return DoExecute(params_binder, connection);
    } catch (const std::exception& err) {
        AccountQueryFailed(connection);
        throw;
    }
}

template <typename T>
ResultSet Client::ExecuteDecompose(OperationType operation_type, const Query& query, const T& row) const {
    auto connection = GetConnection(operation_type);
    AccountQueryExecute(connection);
    try {
        auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(query.GetStatement(), *connection, row);
        return DoExecute(params_binder, connection);
    } catch (const std::exception& err) {
        AccountQueryFailed(connection);
        throw;
    }
}

template <typename Container>
void Client::ExecuteMany(OperationType operation_type, const Query& query, const Container& params) const {
    auto connection = GetConnection(operation_type);
    for (const auto& row : params) {
        AccountQueryExecute(connection);
        try {
            auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(query.GetStatement(), *connection, row);
            DoExecute(params_binder, connection);
        } catch (const std::exception& err) {
            AccountQueryFailed(connection);
            throw;
        }
    }
}

template <typename T, typename... Args>
CursorResultSet<T>
Client::GetCursor(OperationType operation_type, std::size_t batch_size, const Query& query, const Args&... args) const {
    auto connection = GetConnection(operation_type);
    AccountQueryExecute(connection);
    try {
        auto params_binder = impl::BindHelper::UpdateParamsBindings(query.GetStatement(), *connection, args...);
        return CursorResultSet<T>{DoExecute(params_binder, connection), batch_size};
    } catch (const std::exception& err) {
        AccountQueryFailed(connection);
        throw;
    }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END

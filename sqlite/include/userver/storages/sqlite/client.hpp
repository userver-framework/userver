#pragma once

/// @file userver/storages/sqlite/client.hpp
/// @brief @copybrief storages::sqlite::Client

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
/// @brief Interface for executing queries on SQLite connection.
/// Usually retrieved from components::SQLite
class Client final {
public:
    /// @brief Client constructor
    Client(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor);

    /// @brief Client destructor
    ~Client();

    /// @brief Executes a statement with provided operation type.
    ///
    /// Fills placeholders of the statement with args..., `Args` are expected to
    /// be of supported types.
    /// See @ref scripts/docs/en/userver/sqlite/supported_types.md for better understanding of `Args`
    /// requirements.
    ///
    /// @tparam Args Types of parameters to bind
    /// @param operation_type Type of the operation (e.g., Read, Write)
    /// @param query SQL query to execute
    /// @param args Parameters to bind to the query
    /// @return ResultSet containing the results of the query
    template <typename... Args>
    ResultSet Execute(OperationType operation_type, const Query& query, const Args&... args) const;

    /// @brief Executes a statement with provided operation type, decomposing the row.
    ///
    /// Decomposes the fields of the row and binds them as parameters to the query.
    /// See @ref scripts/docs/en/userver/sqlite/supported_types.md for better understanding of `T` requirements.
    ///
    /// @tparam T Type of the row to decompose
    /// @param operation_type Type of the operation (e.g., Read, Write)
    /// @param query SQL query to execute
    /// @param row Row object to decompose and bind
    /// @return ResultSet containing the results of the query
    template <typename T>
    ResultSet ExecuteDecompose(OperationType operation_type, const Query& query, const T& row) const;

    /// @brief Executes a statement multiple times with provided operation type.
    ///
    /// Iterates over the container and executes the query for each element.
    /// Container is expected to be a std::Container, Container::value_type is
    /// expected to be an aggregate of supported types.
    /// See @ref scripts/docs/en/userver/mysql/supported_types.md for better understanding of
    /// `Container::value_type` requirements.
    ///
    /// @tparam Container Type of the container holding rows
    /// @param operation_type Type of the operation (e.g., Read, Write)
    /// @param query SQL query to execute
    /// @param params Container of rows to bind and execute
    template <typename Container>
    void ExecuteMany(OperationType operation_type, const Query& query, const Container& params) const;

    /// @brief Begins a transaction with specified operation type and options.
    ///
    /// @param operation_type Type of the operation (e.g., Read, Write)
    /// @param options Transaction options
    /// @return Transaction object representing the started transaction
    Transaction Begin(OperationType operation_type, const settings::TransactionOptions& options) const;

    /// @brief Creates a savepoint with specified operation type and name.
    ///
    /// @param operation_type Type of the operation (e.g., Read, Write)
    /// @param name Name of the savepoint
    /// @return Savepoint object representing the created savepoint
    Savepoint Save(OperationType operation_type, std::string name) const;

    /// @brief Executes a statement and returns a lazy cursor for batched result retrieval.
    ///
    /// This cursor combines execution and iteration: it fetches up to `batch_size` rows
    /// on each step, loading data on demand rather than all at once. Placeholders in the
    /// statement are filled with `args...`, which must be of supported types.
    /// See @ref scripts/docs/en/userver/sqlite/supported_types.md for detailed mapping of `Args`.
    ///
    /// @tparam T    Type of each result row
    /// @tparam Args Parameter types to bind
    /// @param operation_type Hint for selecting the connection pool (e.g., ReadOnly, ReadWrite)
    /// @param batch_size     Number of rows to fetch per iteration
    /// @param query          SQL query to execute
    /// @param args           Parameters to bind to the query
    /// @return CursorResultSet<T> lazily delivering rows in batches
    template <typename T, typename... Args>
    CursorResultSet<T>
    GetCursor(OperationType operation_type, std::size_t batch_size, const Query& query, const Args&... args) const;

    /// @brief Writes client statistics to the provided writer.
    ///
    /// @param writer Statistics writer to output the statistics
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

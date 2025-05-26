#pragma once

/// @file userver/storages/sqlite/savepoint.hpp

#include <memory>

#include <userver/storages/sqlite/cursor_result_set.hpp>
#include <userver/storages/sqlite/impl/binder_help.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief RAII savepoint wrapper, auto-<b>ROLLBACK</b>s on destruction if no
/// prior `Release`/`Rollback` call was made.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::sqlite::Client
class Savepoint final {
public:
    Savepoint(std::shared_ptr<infra::ConnectionPtr> connection, std::string name);
    ~Savepoint();
    Savepoint(const Savepoint& other) = delete;
    Savepoint(Savepoint&& other) noexcept;
    Savepoint& operator=(Savepoint&&) noexcept;

    /// @brief Executes a statement.
    ///
    /// Fills placeholders of the statement with args..., `Args` are expected to
    /// be of supported types.
    /// See @ref scripts/docs/en/userver/sqlite/supported_types.md for better understanding of `Args`
    /// requirements.
    ///
    /// @tparam Args Types of parameters to bind
    /// @param query SQL query to execute
    /// @param args Parameters to bind to the query
    /// @return ResultSet containing the results of the query
    template <typename... Args>
    ResultSet Execute(const Query& query, const Args&... args) const;

    /// @brief Executes a statement, decomposing the row.
    ///
    /// Decomposes the fields of the row and binds them as parameters to the query.
    /// See @ref scripts/docs/en/userver/sqlite/supported_types.md for better understanding of `T` requirements.
    ///
    /// @tparam T Type of the row to decompose
    /// @param query SQL query to execute
    /// @param row Row object to decompose and bind
    /// @return ResultSet containing the results of the query
    template <typename T>
    ResultSet ExecuteDecompose(const Query& query, const T& row) const;

    /// @brief Executes a statement multiple times.
    ///
    /// Iterates over the container and executes the query for each element.
    /// Container is expected to be a std::Container, Container::value_type is
    /// expected to be an aggregate of supported types.
    /// See @ref scripts/docs/en/userver/mysql/supported_types.md for better understanding of
    /// `Container::value_type` requirements.
    ///
    /// @tparam Container Type of the container holding rows
    /// @param query SQL query to execute
    /// @param params Container of rows to bind and execute
    template <typename Container>
    void ExecuteMany(const Query& query, const Container& params) const;

    /// @brief Executes a statement and returns a cursor for iterating over results.
    ///
    /// Fills placeholders of the statement with args..., `Args` are expected to
    /// be of supported types.
    /// See @ref scripts/docs/en/userver/sqlite/supported_types.md for better understanding of `Args`
    /// requirements.
    ///
    /// @tparam T Type of the result rows
    /// @tparam Args Types of parameters to bind
    /// @param batch_size Number of rows to fetch per batch
    /// @param query SQL query to execute
    /// @param args Parameters to bind to the query
    /// @return CursorResultSet for iterating over the results
    template <typename T, typename... Args>
    CursorResultSet<T> GetCursor(std::size_t batch_size, const Query& query, const Args&... args) const;

    /// @brief Creates a wrapped savepoint with specified name.
    ///
    /// @param name Name of the savepoint
    /// @return Savepoint object representing the created savepoint
    Savepoint Save(std::string name) const;

    /// @brief Release the savepint
    void Release();

    /// @brief Rollback the savepint
    void RollbackTo();

private:
    ResultSet DoExecute(impl::io::ParamsBinderBase& params) const;
    std::string PrepareString(const std::string& str);
    void AssertValid() const;

    void AccountQueryExecute() const noexcept;
    void AccountQueryFailed() const noexcept;

    std::shared_ptr<infra::ConnectionPtr> connection_;
    std::string name_;
};

template <typename... Args>
ResultSet Savepoint::Execute(const Query& query, const Args&... args) const {
    AssertValid();
    AccountQueryExecute();
    try {
        auto params_binder = impl::BindHelper::UpdateParamsBindings(query.GetStatement(), *connection_, args...);
        return DoExecute(params_binder);
    } catch (const std::exception& err) {
        AccountQueryFailed();
        throw;
    }
}

template <typename T>
ResultSet Savepoint::ExecuteDecompose(const Query& query, const T& row) const {
    AssertValid();
    AccountQueryExecute();
    try {
        auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(query.GetStatement(), *connection_, row);
        return DoExecute(params_binder);
    } catch (const std::exception& err) {
        AccountQueryFailed();
        throw;
    }
}

template <typename Container>
void Savepoint::ExecuteMany(const Query& query, const Container& params) const {
    AssertValid();
    for (const auto& row : params) {
        AccountQueryExecute();
        try {
            auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(query.GetStatement(), *connection_, row);
            DoExecute(params_binder);
        } catch (const std::exception& err) {
            AccountQueryFailed();
            throw;
        }
    }
}

template <typename T, typename... Args>
CursorResultSet<T> Savepoint::GetCursor(std::size_t batch_size, const Query& query, const Args&... args) const {
    AssertValid();
    AccountQueryExecute();
    try {
        auto params_binder = impl::BindHelper::UpdateParamsBindings(query.GetStatement(), *connection_, args...);
        return CursorResultSet<T>{DoExecute(params_binder, connection_), batch_size};
    } catch (const std::exception& err) {
        AccountQueryFailed();
        throw;
    }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END

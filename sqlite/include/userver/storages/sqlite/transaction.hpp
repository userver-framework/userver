#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/sqlite/cursor_result_set.hpp>
#include <userver/storages/sqlite/impl/binder_help.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/savepoint.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Transaction final {
public:
    Transaction(std::shared_ptr<infra::ConnectionPtr> connection, const settings::TransactionOptions& options);
    ~Transaction();
    Transaction(const Transaction& other) = delete;
    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&&) noexcept;

    template <typename... Args>
    ResultSet Execute(const Query& query, const Args&... args) const;

    template <typename T>
    ResultSet ExecuteDecompose(const Query& query, const T& row) const;

    template <typename Container>
    void ExecuteMany(const Query& query, const Container& params) const;

    template <typename T, typename... Args>
    CursorResultSet<T> GetCursor(std::size_t batch_size, const Query& query, const Args&... args) const;

    Savepoint Save(std::string name) const;

    void Commit();

    void Rollback();

private:
    ResultSet DoExecute(impl::io::ParamsBinderBase& params) const;
    void AssertValid() const;

    void AccountQueryExecute() const noexcept;
    void AccountQueryFailed() const noexcept;

    std::shared_ptr<infra::ConnectionPtr> connection_;
};

template <typename... Args>
ResultSet Transaction::Execute(const Query& query, const Args&... args) const {
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
ResultSet Transaction::ExecuteDecompose(const Query& query, const T& row) const {
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
void Transaction::ExecuteMany(const Query& query, const Container& params) const {
    AssertValid();
    AccountQueryExecute();
    for (const auto& row : params) {
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
CursorResultSet<T> Transaction::GetCursor(std::size_t batch_size, const Query& query, const Args&... args) const {
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

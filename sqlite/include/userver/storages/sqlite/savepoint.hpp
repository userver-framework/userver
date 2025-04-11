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

class Savepoint final {
public:
    Savepoint(std::shared_ptr<infra::ConnectionPtr> connection, std::string name);
    ~Savepoint();
    Savepoint(const Savepoint& other) = delete;
    Savepoint(Savepoint&& other) noexcept;
    Savepoint& operator=(Savepoint&&) noexcept;

    template <typename... Args>
    ResultSet Execute(const Query& query, const Args&... args) const;

    template <typename T>
    ResultSet ExecuteDecompose(const Query& query, const T& row) const;

    template <typename Container>
    void ExecuteMany(const Query& query, const Container& params) const;

    template <typename T, typename... Args>
    CursorResultSet<T> GetCursor(std::size_t batch_size, const Query& query, const Args&... args) const;

    Savepoint Save(std::string name) const;

    void Release();

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

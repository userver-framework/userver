#pragma once

#include <chrono>
#include <cstddef>
#include <memory>

#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Connection;
class ResultSet;

namespace detail {

class ConnectionPtr;
class Pool;

class CursorImpl final {
public:
    static std::shared_ptr<CursorImpl> Create(
        ConnectionPtr&& connection,
        std::shared_ptr<Pool> pool,
        Query query,
        const impl::ParameterList& parameters,
        std::chrono::milliseconds network_timeout,
        std::chrono::milliseconds statement_timeout,
        bool in_transaction
    );

    static std::shared_ptr<CursorImpl> Create(
        Connection& connection,
        std::shared_ptr<Pool> pool,
        Query query,
        const impl::ParameterList& parameters,
        std::chrono::milliseconds network_timeout,
        std::chrono::milliseconds statement_timeout,
        bool in_transaction
    );

    ~CursorImpl() noexcept;

    ResultSet Fetch(std::size_t rows);
    bool Done() const noexcept;
    std::size_t FetchedSoFar() const noexcept;

private:
    struct Impl;

    explicit CursorImpl(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;
};

}  // namespace detail

}  // namespace storages::odbc

USERVER_NAMESPACE_END

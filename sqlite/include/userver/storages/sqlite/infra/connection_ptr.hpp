#pragma once

/// @file userver/storages/sqlite/infra/connection_ptr.hpp
/// @brief @copybrief storages::sqlite::infra::ConnectionPtr

#include <memory>

#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

/// @brief Owns a SQLite connection and keeps its parent pool alive
class ConnectionPtr {
public:
    ConnectionPtr(std::shared_ptr<Pool>&& pool, std::unique_ptr<impl::Connection>&& connection);
    ~ConnectionPtr();

    ConnectionPtr(ConnectionPtr&&) noexcept;
    ConnectionPtr& operator=(ConnectionPtr&&) noexcept;

    bool IsValid() const noexcept;
    impl::Connection* get() const noexcept;

    impl::Connection& operator*() const;
    impl::Connection* operator->() const noexcept;

private:
    void Reset(std::unique_ptr<impl::Connection> conn, std::shared_ptr<Pool> pool);
    void Release();

    std::shared_ptr<Pool> pool_;
    std::unique_ptr<impl::Connection> conn_;
};

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END

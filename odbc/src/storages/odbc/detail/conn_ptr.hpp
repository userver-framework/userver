#pragma once

#include <memory>

#include <storages/odbc/detail/connection.hpp>
#include <userver/storages/odbc/odbc_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ConnectionPtr {
public:
    ConnectionPtr(std::shared_ptr<Pool>&& pool, std::unique_ptr<Connection>&& connection);

    ~ConnectionPtr();

    ConnectionPtr(ConnectionPtr&& other) noexcept;
    ConnectionPtr& operator=(ConnectionPtr&& other) noexcept;

    bool IsValid() const noexcept;
    Connection* get() const noexcept;

    Connection& operator*() const;
    Connection* operator->() const noexcept;

private:
    void Reset(std::unique_ptr<Connection> conn, std::shared_ptr<Pool> pool);
    void Release();

    std::shared_ptr<Pool> pool_;
    std::unique_ptr<Connection> conn_;
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

#pragma once

#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/connection.hpp>
#include <string>
#include <userver/drivers/impl/connection_pool_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class Pool final : public drivers::impl::ConnectionPoolBase<Connection, Pool> {
public:
    Pool(const std::string& dsn, std::size_t max_pool_size, std::size_t max_simultaneously_connecting_clients);

    ~Pool();

    ConnectionPtr Acquire();

    void Release(ConnectionUniquePtr connection);

private:
    friend class drivers::impl::ConnectionPoolBase<Connection, Pool>;

    ConnectionUniquePtr DoCreateConnection(engine::Deadline deadline);

    void AccountConnectionCreated() noexcept;
    void AccountConnectionAcquired() noexcept;
    void AccountConnectionReleased() noexcept;
    void AccountConnectionDestroyed() noexcept;
    void AccountOverload() noexcept;

    std::string dsn_;
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

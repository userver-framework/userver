#pragma once

#include <atomic>
#include <cstddef>
#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/statistics.hpp>
#include <string>
#include <vector>
#include <userver/drivers/impl/connection_pool_base.hpp>
#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class Pool final : public drivers::impl::ConnectionPoolBase<Connection, Pool> {
public:
    Pool(const std::string& dsn, std::size_t min_pool_size, std::size_t max_pool_size);
    Pool(std::vector<std::string> dsns, std::size_t min_pool_size, std::size_t max_pool_size);

    ~Pool();

    ConnectionPtr Acquire(engine::Deadline deadline);

    void Release(ConnectionUniquePtr connection);

    const PoolConnectionStatistics& GetConnectionStatistics() const noexcept { return stats_; }

private:
    friend class drivers::impl::ConnectionPoolBase<Connection, Pool>;

    ConnectionUniquePtr DoCreateConnection(engine::Deadline deadline);

    void AccountConnectionCreated() noexcept;
    void AccountConnectionAcquired() noexcept;
    void AccountConnectionReleased() noexcept;
    void AccountConnectionDestroyed() noexcept;
    void AccountOverload() noexcept;

    const std::vector<std::string> dsns_;
    mutable std::atomic<std::size_t> dsn_index_{0};

    PoolConnectionStatistics stats_{};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

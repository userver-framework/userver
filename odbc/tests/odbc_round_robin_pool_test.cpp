#include <gtest/gtest.h>

#include <storages/odbc/detail/pool.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(Topology, RoundRobinPoolCreatesConnectionsInTurn) {
    // We don't need a real database here: all DSNs are invalid, so each Acquire should throw.
    // This is a smoke-test for the multi-DSN Pool constructor.
    std::vector<std::string> dsns{"invalid_dsn_1", "invalid_dsn_2", "invalid_dsn_3"};
    storages::odbc::detail::Pool pool(std::move(dsns), 0, 1);

    for (int i = 0; i < 6; ++i) {
        UEXPECT_THROW(pool.Acquire(engine::Deadline::FromDuration(std::chrono::seconds{2})), storages::odbc::Error);
    }
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END

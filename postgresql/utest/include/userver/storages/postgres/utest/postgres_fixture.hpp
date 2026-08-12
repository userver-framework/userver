#pragma once

/// @file userver/storages/postgres/utest/postgres_fixture.hpp
/// @brief @copybrief storages::postgres::utest::PostgresTest

#include <userver/utest/utest.hpp>

#include <userver/storages/postgres/utest/cluster_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utest {

/// Postgres fixture
///
/// @brief Provides access to `storages::postgres::Cluster` by means of the
/// `storages::postgres::utest::ClusterLocal` class
///
/// The cluster connects to the DSN from the `POSTGRES_TEST_DSN` environment
/// variable. Schema setup and teardown is left to the derived fixture.
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class PostgresTest : public ::testing::Test, public ClusterLocal {
public:
    PostgresTest() = default;
};

}  // namespace storages::postgres::utest

USERVER_NAMESPACE_END

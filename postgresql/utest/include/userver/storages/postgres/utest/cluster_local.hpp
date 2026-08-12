#pragma once

/// @file userver/storages/postgres/utest/cluster_local.hpp
/// @brief @copybrief storages::postgres::utest::ClusterLocal

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/testsuite/tasks.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utest {

/// @brief Make default cluster settings for postgres local
/// @note This configuration should be used for testing purposes
storages::postgres::ClusterSettings MakeDefaultClusterSettings();

/// @brief Postgres local class
///
/// @brief Owns a `storages::postgres::Cluster` connected to the DSN from the
/// `POSTGRES_TEST_DSN` environment variable, together with the
/// `testsuite::TestsuiteTasks` the cluster depends on.
///
/// @note Must be used from a coroutine context (e.g. inside a `UTEST` body),
/// as the cluster is created on the current task processor.
class ClusterLocal {
public:
    /// Postgres local default constructor (use default settings)
    ClusterLocal();

    /// Postgres local constructor with specified settings
    explicit ClusterLocal(const storages::postgres::ClusterSettings& settings);

    ~ClusterLocal();

    /// Get cluster
    const std::shared_ptr<storages::postgres::Cluster>& GetCluster() const;

    /// @brief Recreate the cluster with the same settings.
    ///
    /// Drops the current connection pool and establishes a new one. Useful when
    /// the database schema is changed at runtime (e.g. migrations applied inside
    /// a test): freshly opened connections reload the user types, so composite
    /// and enum types created after the original cluster can be (de)serialized.
    void RenewCluster();

private:
    testsuite::TestsuiteTasks testsuite_tasks_{true};
    storages::postgres::ClusterSettings settings_;
    std::shared_ptr<storages::postgres::Cluster> cluster_;
};

}  // namespace storages::postgres::utest

USERVER_NAMESPACE_END

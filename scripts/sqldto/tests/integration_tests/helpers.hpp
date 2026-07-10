#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/utest/postgres_fixture.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/resources.hpp>

#include <input/pg_cluster.hpp>

namespace sqldto_tests {

namespace pg = USERVER_NAMESPACE::storages::postgres;

// Name the migration is embedded under in CMakeLists.txt (userver_embed_file).
inline constexpr std::string_view kSchemaResource = "input/V001__schema.sql";

inline const std::string kCleanupSql = "DROP SCHEMA IF EXISTS smoke CASCADE";

inline constexpr pg::CommandControl kMigrationCommandControl{
    std::chrono::seconds{2},
    std::chrono::milliseconds{500},
    pg::CommandControl::PreparedStatementsOptionOverride::kDisabled,
};

// The embedded migration is a single script
inline std::vector<std::string_view> SplitStatements(std::string_view sql) {
    std::vector<std::string_view> statements;
    std::size_t start = 0;
    while (start < sql.size()) {
        const auto end = sql.find(';', start);
        const auto stop = end == std::string_view::npos ? sql.size() : end;

        auto statement = sql.substr(start, stop - start);
        const auto first = statement.find_first_not_of(" \t\r\n");
        if (first != std::string_view::npos) {
            const auto last = statement.find_last_not_of(" \t\r\n");
            statements.push_back(statement.substr(first, last - first + 1));
        }

        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return statements;
}

inline void ApplyMigrations(pg::Transaction& trx) {
    const auto schema = USERVER_NAMESPACE::utils::FindResource(kSchemaResource);
    for (const auto statement : SplitStatements(schema)) {
        trx.Execute(kMigrationCommandControl, std::string{statement});
    }
}

// Base fixture: applies the embedded schema before the test and drops it after.
//
// SetUp/TearDown are run inside the coroutine context by UTEST_F, so they may
// freely use the cluster, just like the test body.
class PgSmokeTestBase : public pg::utest::PostgresTest {
protected:
    void SetUp() override {
        cluster_ = GetCluster();
        auto trx =
            cluster_->Begin(pg::ClusterHostType::kMaster, pg::TransactionOptions{pg::TransactionOptions::kReadWrite});
        trx.Execute(kMigrationCommandControl, kCleanupSql);
        ApplyMigrations(trx);
        trx.Commit();

        RenewCluster();
        cluster_ = GetCluster();

        client_ = std::make_unique<input::ClusterPgClient>(cluster_);
    }

    void TearDown() override {
        client_.reset();
        if (cluster_) {
            auto trx =
                cluster_
                    ->Begin(pg::ClusterHostType::kMaster, pg::TransactionOptions{pg::TransactionOptions::kReadWrite});
            trx.Execute(kMigrationCommandControl, kCleanupSql);
            trx.Commit();
            cluster_.reset();
        }
    }

    std::shared_ptr<pg::Cluster> cluster_;
    std::unique_ptr<input::ClusterPgClient> client_;
};

}  // namespace sqldto_tests

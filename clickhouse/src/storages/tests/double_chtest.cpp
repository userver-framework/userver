#include <userver/utest/utest.hpp>

#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithDoubles final {
    std::vector<double> doubles;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithDoubles> {
    using mapped_type = std::tuple<columns::Float64Column>;
};

}  // namespace storages::clickhouse::io

UTEST(Double, InsertSelectWhere) {
    ClusterWrapper cluster{};
    cluster->Execute(
        "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
        "(value Float64)"
    );

    const DataWithDoubles insert_data{
        {-std::numeric_limits<double>::min(), -1, 0, 1, std::numeric_limits<double>::max()}};
    cluster->Insert("tmp_table", {"value"}, insert_data);

    const storages::clickhouse::Query query{"SELECT * FROM tmp_table WHERE value < {0}"};

    const auto select_data =
        cluster->Execute(query, storages::clickhouse::io::FloatingWithPrecision<double, 2>(0.5)).As<DataWithDoubles>();
    ASSERT_EQ(select_data.doubles.size(), 3);

    for (std::size_t i = 0; i < 2; ++i) {
        ASSERT_TRUE(select_data.doubles[i] < 0.5);
    }
}

USERVER_NAMESPACE_END

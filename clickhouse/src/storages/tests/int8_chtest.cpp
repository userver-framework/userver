#include <userver/utest/utest.hpp>

#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithInts final {
    std::vector<int8_t> ints;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithInts> {
    using mapped_type = std::tuple<columns::Int8Column>;
};

}  // namespace storages::clickhouse::io

UTEST(Int8, InsertSelect) {
    ClusterWrapper cluster{};
    cluster->Execute(
        "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
        "(value Int8)"
    );

    const DataWithInts insert_data{{std::numeric_limits<int8_t>::min(), -1, 0, 1, std::numeric_limits<int8_t>::max()}};
    cluster->Insert("tmp_table", {"value"}, insert_data);

    const auto select_data = cluster->Execute("SELECT * from tmp_table").As<DataWithInts>();
    ASSERT_EQ(select_data.ints.size(), 5);

    for (size_t i = 0; i < insert_data.ints.size(); ++i) {
        ASSERT_EQ(insert_data.ints[i], select_data.ints[i]);
    }
}

USERVER_NAMESPACE_END

#include <userver/utest/utest.hpp>

#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithUInts final {
    std::vector<uint32_t> ints;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithUInts> {
    using mapped_type = std::tuple<columns::UInt32Column>;
};

}  // namespace storages::clickhouse::io

UTEST(UInt32, InsertSelect) {
    ClusterWrapper cluster{};
    cluster->Execute(
        "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
        "(value UInt32)"
    );

    const DataWithUInts insert_data{{0, 1, 10, std::numeric_limits<uint32_t>::max()}};
    cluster->Insert("tmp_table", {"value"}, insert_data);

    const auto select_data = cluster->Execute("SELECT * from tmp_table").As<DataWithUInts>();
    ASSERT_EQ(select_data.ints.size(), 4);

    for (size_t i = 0; i < insert_data.ints.size(); ++i) {
        ASSERT_EQ(insert_data.ints[i], select_data.ints[i]);
    }
}

USERVER_NAMESPACE_END

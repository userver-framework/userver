#include <userver/utest/utest.hpp>

#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithFloats final {
  std::vector<float> floats;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithFloats> {
  using mapped_type = std::tuple<columns::Float32Column>;
};

}  // namespace storages::clickhouse::io

UTEST(Float32, InsertSelect) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(value Float32)");

  const DataWithFloats insert_data{{1.2, 2.3, 3.4}};
  cluster->Insert("tmp_table", {"value"}, insert_data);

  const auto select_data =
      cluster->Execute("SELECT * from tmp_table").As<DataWithFloats>();
  ASSERT_EQ(select_data.floats.size(), 3);

  for (size_t i = 0; i < insert_data.floats.size(); ++i) {
    ASSERT_FLOAT_EQ(insert_data.floats[i], select_data.floats[i]);
  }
}

USERVER_NAMESPACE_END

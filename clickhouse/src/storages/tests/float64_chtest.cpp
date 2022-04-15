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

UTEST(Float64, InsertSelect) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(value Float64)");

  const DataWithDoubles insert_data{{3.141592, 1.4142, 10.101010}};
  cluster->Insert("tmp_table", {"value"}, insert_data);

  const auto select_data =
      cluster->Execute("SELECT * from tmp_table").As<DataWithDoubles>();
  ASSERT_EQ(select_data.doubles.size(), 3);

  for (size_t i = 0; i < insert_data.doubles.size(); ++i) {
    ASSERT_DOUBLE_EQ(insert_data.doubles[i], select_data.doubles[i]);
  }
}

USERVER_NAMESPACE_END

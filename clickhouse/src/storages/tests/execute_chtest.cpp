#include <userver/utest/utest.hpp>

#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

const storages::clickhouse::Query common_query{
    "SELECT c.number, randomString(10), c.number as t, NOW64(9) "
    "FROM "
    "numbers(0, 10000) c "};
}

/// [Sample CppToClickhouse specialization]
namespace {

struct Data final {
  std::vector<uint64_t> numbers;
  std::vector<std::string> strings;
  std::vector<uint64_t> other_numbers;
  std::vector<std::chrono::system_clock::time_point> tps;
};

struct RowData final {
  uint64_t number;
  std::string string;
  uint64_t other_number;
  std::chrono::system_clock::time_point tp;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<Data> final {
  using mapped_type =
      std::tuple<columns::UInt64Column, columns::StringColumn,
                 columns::UInt64Column, columns::DateTime64ColumnNano>;
};

template <>
struct CppToClickhouse<RowData> final {
  using mapped_type =
      std::tuple<columns::UInt64Column, columns::StringColumn,
                 columns::UInt64Column, columns::DateTime64ColumnNano>;
};

}  // namespace storages::clickhouse::io
/// [Sample CppToClickhouse specialization]

UTEST(Execute, MappingWorks) {
  ClusterWrapper cluster{};

  /// [Sample ExecutionResult usage]
  const Data as_columns{cluster->Execute(common_query).As<Data>()};
  const std::vector<RowData> as_rows{
      cluster->Execute(common_query).AsContainer<std::vector<RowData>>()};
  const auto as_rows_iterable{cluster->Execute(common_query).AsRows<RowData>()};
  /// [Sample ExecutionResult usage]

  EXPECT_EQ(as_columns.numbers.size(), 10000);
  EXPECT_EQ(as_columns.numbers[5001], 5001);

  EXPECT_EQ(as_rows.size(), 10000);
  EXPECT_EQ(as_rows[5001].number, 5001);

  uint64_t sum = 0;
  for (auto&& data : as_rows_iterable) {
    sum += data.number;
  }
  EXPECT_EQ(sum, 10000 * (10000 - 1) / 2);
}

namespace {
namespace io = storages::clickhouse::io;

template <typename Row>
class IteratorTester final {
 public:
  template <size_t Index, typename U>
  static void CheckCurrentValue(
      typename io::RowsMapper<Row>::Iterator& iterator, U value) {
    const auto& iterators =
        io::IteratorsTester::GetCurrentIteratorsTuple<Row>(iterator);
    ASSERT_EQ(*std::get<Index>(iterators), value);
  }
};
}  // namespace

UTEST(Execute, IterationMovesFromUnderlying) {
  static_assert(io::IteratorsTester::kCanMoveFromIterators<RowData>);

  ClusterWrapper cluster{};

  const size_t limit = 10;
  const storages::clickhouse::Query q{
      "SELECT c.number, repeat(toString(c.number), 100), toUInt64(1), NOW64(9) "
      "FROM system.numbers c LIMIT 10"};

  std::vector<std::string> expected;
  expected.reserve(limit);
  for (size_t i = 0; i < limit; ++i) expected.emplace_back(100, '0' + i);

  auto ch_res = cluster->Execute(q);
  ASSERT_EQ(ch_res.GetRowsCount(), limit);
  auto res = std::move(ch_res).AsRows<RowData>();

  size_t ind = 0;
  for (auto it = res.begin(); it != res.end(); ++it, ++ind) {
    ASSERT_LT(ind, limit);
    IteratorTester<RowData>::CheckCurrentValue<1>(it, expected[ind]);
    ASSERT_EQ(it->string, expected[ind]);
    IteratorTester<RowData>::CheckCurrentValue<1>(it, std::string{});
    ASSERT_EQ(it->string, expected[ind]);
    [[maybe_unused]] std::string tmp{std::move(it->string)};
    ASSERT_TRUE(it->string.empty());
  }
}

USERVER_NAMESPACE_END

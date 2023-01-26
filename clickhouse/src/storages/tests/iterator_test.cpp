#include <userver/utest/utest.hpp>

#include <userver/storages/clickhouse.hpp>

#include "utils_test.hpp"

#include <clickhouse/columns/string.h>

namespace {
namespace io = USERVER_NAMESPACE::storages::clickhouse::io;
namespace columns = io::columns;
namespace clickhouse_cpp = clickhouse;

template <typename... Args>
struct IteratorDefaultConstructorInstantiator final {
  ~IteratorDefaultConstructorInstantiator() {
    (..., typename Args::iterator{});
    (..., typename columns::NullableColumn<Args>::iterator{});
  }
};

[[maybe_unused]] const IteratorDefaultConstructorInstantiator<
    columns::Int8Column, columns::UInt8Column, columns::UInt16Column,
    columns::UInt32Column, columns::UInt64Column, columns::UuidColumn,
    columns::DateTimeColumn, columns::DateTime64ColumnMilli,
    columns::DateTime64ColumnMicro, columns::DateTime64ColumnNano,
    columns::StringColumn, columns::Float32Column, columns::Float64Column>
    validator{};

}  // namespace

USERVER_NAMESPACE_BEGIN

namespace {
class IteratorTester final {
 public:
  static void CheckCurrentValue(
      columns::ColumnIterator<columns::StringColumn>& iterator,
      std::optional<std::string> value) {
    EXPECT_EQ(io::IteratorsTester::GetCurrentValue(iterator), value);
  }
};
}  // namespace

TEST(StringIterator, ResetsCurrentValue) {
  std::string first_string(100, 'a');
  std::string second_string(100, 'b');

  columns::StringColumn column{std::make_shared<clickhouse_cpp::ColumnString>(
      std::vector{first_string, second_string})};
  ASSERT_EQ(column.Size(), 2);

  auto current = column.begin();
  IteratorTester::CheckCurrentValue(current, std::nullopt);
  EXPECT_EQ(*current, first_string);
  IteratorTester::CheckCurrentValue(current, first_string);

  auto begin = current++;
  IteratorTester::CheckCurrentValue(begin, first_string);
  IteratorTester::CheckCurrentValue(current, std::nullopt);
  EXPECT_EQ(*current, second_string);
  IteratorTester::CheckCurrentValue(current, second_string);
}

USERVER_NAMESPACE_END

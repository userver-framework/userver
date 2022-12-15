#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

#include <userver/decimal64/decimal64.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

using Decimal = decimal64::Decimal<3>;

UTEST(Decimal, Works) {
  TmpTable table{"Value DECIMAL(10, 3) NOT NULL"};

  struct Row final {
    Decimal decimal{};
  };
  const Decimal value_to_insert{"123123.123"};

  table.DefaultExecute("INSERT INTO {} VALUES(?)", value_to_insert);

  const auto db_row =
      table.DefaultExecute("SELECT Value FROM {}").AsSingleRow<Row>();
  EXPECT_EQ(value_to_insert, db_row.decimal);
}

UTEST(Decimal, OptionalWorks) {
  TmpTable table{"Value DECIMAL(10, 3)"};

  struct Row final {
    std::optional<Decimal> decimal;
  };

  table.DefaultExecute("INSERT INTO {} VALUES(?)", std::optional<Decimal>{});

  const auto db_row =
      table.DefaultExecute("SELECT Value FROM {}").AsSingleRow<Row>();
  EXPECT_FALSE(db_row.decimal.has_value());
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END

#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

#include <userver/decimal64/decimal64.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

using Decimal = decimal64::Decimal<3>;

UTEST(Decimal, FetchIntoRealFromReal) {
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

UTEST(Decimal, FetchIntoOptionalFromNull) {
  TmpTable table{"Value DECIMAL(10, 3)"};

  struct Row final {
    std::optional<Decimal> decimal;
  };

  table.DefaultExecute("INSERT INTO {} VALUES(?)", std::optional<Decimal>{});

  const auto db_row =
      table.DefaultExecute("SELECT Value FROM {}").AsSingleRow<Row>();
  EXPECT_FALSE(db_row.decimal.has_value());
}

UTEST(Decimal, FetchIntoOptionalFromEngagedNullable) {
  TmpTable table{"Value DECIMAL(10, 3)"};

  const std::optional<Decimal> decimal_to_insert{1};
  table.DefaultExecute("INSERT INTO {} VALUES(?)", decimal_to_insert);

  const auto decimal_field = table.DefaultExecute("SELECT Value FROM {}")
                                 .AsSingleField<std::optional<Decimal>>();
  ASSERT_TRUE(decimal_field.has_value());
  EXPECT_EQ(decimal_to_insert, decimal_field);
}

UTEST(Decimal, ThrowsOnFractionalOverflow) {
  TmpTable table{"Value DECIMAL(10, 5) NOT NULL"};

  table.DefaultExecute("INSERT INTO {} VALUES(?)", Decimal{1});

  struct Row final {
    Decimal decimal;
  };

  EXPECT_THROW(table.DefaultExecute("SELECT Value FROM {}").AsSingleRow<Row>(),
               decimal64::DecimalError);
}

UTEST(Decimal, ThrowsOnOverflow) {
  TmpTable table{"Value DECIMAL(30, 3) NOT NULL"};

  table.DefaultExecute("INSERT INTO {} VALUES(?)",
                       Decimal{std::int64_t{1} << 40});

  struct Row final {
    Decimal decimal;
  };

  EXPECT_THROW(
      table.DefaultExecute("SELECT Value * Value FROM {}").AsSingleRow<Row>(),
      decimal64::DecimalError);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END

#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

UTEST_DEATH(OutputBindingDeathTest, NullableToT) {
  TmpTable table{"Id INT"};
  table.DefaultExecute("INSERT INTO {}(Id) VALUES(1)");

  struct Row final {
    std::int32_t id;
  };
  // TODO : can we do it without insert? rn Validate isn't called because there
  // are no binds done
  EXPECT_UINVARIANT_FAILURE(
      table.DefaultExecute("SELECT Id FROM {}").AsVector<Row>());
}

UTEST(OutputBinding, NullAndNotNullCombinations) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, "Id INT NOT NULL, Value INT NOT NULL, Opt INT NULL"};
  table.DefaultExecute("INSERT INTO {}(Id, Value, Opt) VALUES(1, 2, NULL)");
  struct Row final {
    std::int32_t id{};
    std::optional<std::int32_t> value;
    std::optional<std::int32_t> opt;
  };

  const auto row =
      table.DefaultExecute("SELECT Id, Value, Opt FROM {}").AsSingleRow<Row>();
  EXPECT_EQ(row.id, 1);
  EXPECT_EQ(row.value, 2);
  EXPECT_FALSE(row.opt.has_value());
}

UTEST_DEATH(OutputBindingDeathTest, TypeMismatch) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, "Value TEXT"};
  table.DefaultExecute("INSERT INTO {}(Value) VALUES(?)", "some string");

  struct Row final {
    std::chrono::system_clock::time_point tp{};
  };

  EXPECT_UINVARIANT_FAILURE(
      table.DefaultExecute("SELECT Value FROM {}").AsSingleRow<Row>());
}

UTEST_DEATH(OutputBindingDeathTest, SignMismatch) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, "Sig INT NOT NULL, UnSig INT UNSIGNED NOT NULL"};
  table.DefaultExecute("INSERT INTO {} VALUES(1, 1)");

  struct RowUnsigned final {
    std::uint32_t t{};
  };
  struct RowSigned final {
    std::int32_t t{};
  };
  EXPECT_UINVARIANT_FAILURE(
      table.DefaultExecute("SELECT Sig FROM {}").AsSingleRow<RowUnsigned>());
  EXPECT_UINVARIANT_FAILURE(
      table.DefaultExecute("SELECT UnSig FROM {}").AsSingleRow<RowSigned>());
}

UTEST_DEATH(OutputBindingDeathTest, FieldsCountMismatch) {
  ClusterWrapper cluster{};

  struct Row final {
    std::uint32_t id;
    std::uint32_t value;
  };
  EXPECT_UINVARIANT_FAILURE(
      cluster.DefaultExecute("SELECT 1").AsSingleRow<Row>());
  EXPECT_UINVARIANT_FAILURE(
      cluster.DefaultExecute("SELECT 1, 2, 3").AsSingleRow<Row>());
}

UTEST(OutputBinding, AllSupportedDates) {
  ClusterWrapper cluster{};
  TmpTable table{cluster,
                 "DatetimeT DATETIME(6) NOT NULL, DateT DATE NOT NULL, "
                 "TimestampT TIMESTAMP NOT NULL, TimeT TIME NOT NULL"};

  struct AllSupportedDates final {
    std::chrono::system_clock::time_point datetime;
    std::chrono::system_clock::time_point date;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point time;
  };
  const auto now = std::chrono::system_clock::now();

  cluster->InsertOne(
      table.FormatWithTableName("INSERT INTO {} VALUES(?, ?, ?, ?)"),
      AllSupportedDates{now, now, now, now});

  const auto row =
      table.DefaultExecute("SELECT DatetimeT, DateT, TimestampT, TimeT FROM {}")
          .AsSingleRow<AllSupportedDates>();

  const auto to_days = [](std::chrono::system_clock::time_point tp) {
    return std::chrono::time_point_cast<
        std::chrono::duration<long long, std::ratio<86400> > >(tp);
  };

  EXPECT_EQ(ToMariaDBPrecision(now), row.datetime);
  EXPECT_EQ(to_days(now), to_days(row.date));
  // TODO : fix casting
  EXPECT_EQ(std::chrono::time_point_cast<std::chrono::seconds>(now),
            row.timestamp);
}

UTEST(OutputBinding, AllSupportedStrings) {
  ClusterWrapper cluster{};
  TmpTable table{
      cluster,
      "VarCharT VARCHAR(255) NOT NULL, VarBinaryT VARBINARY(255) NOT NULL,"
      "TinyBlobT TINYBLOB NOT NULL, TinyTextT TINYTEXT NOT NULL,"
      "BlobT BLOB NOT NULL, TextT TEXT NOT NULL,"
      "MediumBlobT MEDIUMBLOB NOT NULL, MediumTextT MEDIUMTEXT NOT NULL,"
      "LongBlobT LONGBLOB NOT NULL, LongTextT LONGTEXT NOT NULL"};

  struct AllSupportedStrings final {
    std::string varchar;
    std::string varbinary;
    std::string tinyblob;
    std::string tinytext;
    std::string blob;
    std::string text;
    std::string mediumblob;
    std::string mediumtext;
    std::string longblob;
    std::string longtext;
  };

  const AllSupportedStrings row_to_insert{
      "varchar", "varbinary",  "tinyblob",   "tinytext", "blob",
      "text",    "mediumblob", "mediumtext", "longblob", "longtext"};
  cluster->InsertOne(table.FormatWithTableName(
                         "INSERT INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
                     row_to_insert);

  const auto db_row =
      table
          .DefaultExecute(
              "SELECT VarCharT, VarBinaryT, TinyBlobT, TinyTextT, "
              "BlobT, TextT, "
              "MediumBlobT, MediumTextT, LongBlobT, LongTextT FROM {}")
          .AsSingleRow<AllSupportedStrings>();

  EXPECT_EQ(row_to_insert.varchar, db_row.varchar);
  EXPECT_EQ(row_to_insert.varbinary, db_row.varbinary);
  EXPECT_EQ(row_to_insert.tinyblob, db_row.tinyblob);
  EXPECT_EQ(row_to_insert.tinytext, db_row.tinytext);
  EXPECT_EQ(row_to_insert.blob, db_row.blob);
  EXPECT_EQ(row_to_insert.text, db_row.text);
  EXPECT_EQ(row_to_insert.mediumblob, db_row.mediumblob);
  EXPECT_EQ(row_to_insert.mediumtext, db_row.mediumtext);
  EXPECT_EQ(row_to_insert.longblob, db_row.longblob);
  EXPECT_EQ(row_to_insert.longtext, db_row.longtext);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END

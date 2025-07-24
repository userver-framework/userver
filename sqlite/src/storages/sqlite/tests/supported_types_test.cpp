#include <userver/utest/utest.hpp>

#include <vector>

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <boost/uuid/uuid.hpp>

#include <userver/decimal64/decimal64.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/boost_uuid4.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

class SQLiteTypesTest : public SQLiteCompositeFixture<SQLiteInMemoryConnection> {
    void PreInitialize(const ClientPtr& client) final {
        client->Execute(
            OperationType::kReadWrite,
            R"(CREATE TABLE test(
                              uint8 INTEGER, int8 INTEGER,
                              uint16 INTEGER, int16 INTEGER,
                              uint32 INTEGER, int32 INTEGER,
                              uint64 INTEGER, int64 INTEGER,
                              fl REAL, db REAL,
                              str TEXT, blob BLOB,
                              boolean INTEGER, datetime INTEGER,
                              uuid BLOB, decimal TEXT,
                              json TEXT
                              )
                              )"
        );
    }

    void CleanUp(const ClientPtr& client) final {
        UEXPECT_NO_THROW(client->Execute(OperationType::kReadWrite, "DROP TABLE test"));
    }
};

class SQLiteTypesStringViewTest : public SQLiteCompositeFixture<SQLiteInMemoryConnection> {
    void PreInitialize(const ClientPtr& client) final {
        client->Execute(OperationType::kReadWrite, "CREATE TABLE test(strvw TEXT)");
    }

    void CleanUp(const ClientPtr& client) final { client->Execute(OperationType::kReadWrite, "DROP TABLE test"); }
};

class SQLiteTypesMismatchTest : public SQLiteCompositeFixture<SQLiteInMemoryConnection> {
    void PreInitialize(const ClientPtr& client) override {
        client->Execute(OperationType::kReadWrite, kMismatchTypesTable.data());
    }

    void CleanUp(const ClientPtr& client) final { client->Execute(OperationType::kReadWrite, "DROP TABLE test"); }

protected:
    static constexpr std::string_view kMismatchTypesTable = "CREATE TABLE test(int INTEGER, str TEXT)";
};

class SQLiteTypesStrictMismatchTest : public SQLiteTypesMismatchTest {
    void PreInitialize(const ClientPtr& client) final {
        client->Execute(OperationType::kReadWrite, std::string(kMismatchTypesTable) + " STRICT");
    }
};

constexpr std::string_view kTestJson = R"({
    "key1": 42,
    "key2": {"key3":"val"}
  })";

struct TestRow {
    std::uint8_t uint8{42};
    std::int8_t int8{42};
    std::uint16_t uint16{42};
    std::int16_t int16{42};
    std::uint32_t uint32{42};
    std::int32_t int32{42};
    std::uint64_t uint64{42};
    std::int64_t int64{42};
    float fl{42.42};
    double db{42.42};
    std::string str{"forty two"};
    std::vector<std::uint8_t> blob{str.begin(), str.end()};
    bool boolean{false};
    std::chrono::system_clock::time_point datetime = std::chrono::system_clock::now();
    boost::uuids::uuid uuid = utils::generators::GenerateBoostUuid();
    decimal64::Decimal<4, decimal64::HalfEvenRoundPolicy> decimal{42};
    formats::json::Value json = formats::json::FromString(kTestJson);

    bool operator==(const TestRow& other) const {
        return std::tie(
                   uint8,
                   int8,
                   uint16,
                   int16,
                   uint32,
                   int32,
                   uint64,
                   int64,
                   fl,
                   db,
                   str,
                   blob,
                   boolean,
                   datetime,
                   uuid,
                   decimal,
                   json
               ) ==
               std::tie(
                   other.uint8,
                   other.int8,
                   other.uint16,
                   other.int16,
                   other.uint32,
                   other.int32,
                   other.uint64,
                   other.int64,
                   other.fl,
                   other.db,
                   other.str,
                   other.blob,
                   other.boolean,
                   other.datetime,
                   other.uuid,
                   other.decimal,
                   other.json
               );
    }
};

struct OptionalTestRow {
    std::optional<std::uint8_t> uint8;
    std::optional<std::int8_t> int8;
    std::optional<std::uint16_t> uint16;
    std::optional<std::int16_t> int16;
    std::optional<std::uint32_t> uint32;
    std::optional<std::int32_t> int32;
    std::optional<std::uint64_t> uint64;
    std::optional<std::int64_t> int64;
    std::optional<float> fl;
    std::optional<double> db;
    std::optional<std::string> str;
    std::optional<std::vector<std::uint8_t>> blob;
    std::optional<bool> boolean;
    std::optional<std::chrono::system_clock::time_point> datetime;
    std::optional<boost::uuids::uuid> uuid;
    std::optional<decimal64::Decimal<4, decimal64::HalfEvenRoundPolicy>> decimal;
    std::optional<formats::json::Value> json;

    bool operator==(const OptionalTestRow& other) const {
        return std::tie(
                   uint8,
                   int8,
                   uint16,
                   int16,
                   uint32,
                   int32,
                   uint64,
                   int64,
                   fl,
                   db,
                   str,
                   blob,
                   boolean,
                   datetime,
                   uuid,
                   decimal,
                   json
               ) ==
               std::tie(
                   other.uint8,
                   other.int8,
                   other.uint16,
                   other.int16,
                   other.uint32,
                   other.int32,
                   other.uint64,
                   other.int64,
                   other.fl,
                   other.db,
                   other.str,
                   other.blob,
                   other.boolean,
                   other.datetime,
                   other.uuid,
                   other.decimal,
                   other.json
               );
    }
};

struct MismatchTypesTestRow {
    std::string str{"forty two"};
    int i{42};

    bool operator==(const MismatchTypesTestRow& other) const {
        return std::tie(i, str) == std::tie(other.i, other.str);
    }
};

struct MismatchTypesTestRowCorrect {
    int i{0};
    std::string str{"42"};

    bool operator==(const MismatchTypesTestRowCorrect& other) const {
        return std::tie(i, str) == std::tie(other.i, other.str);
    }
};

}  // namespace

UTEST_F(SQLiteTypesTest, BindExtract) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient());
    TestRow expected_test_row{};
    UEXPECT_NO_THROW(client->ExecuteDecompose(
        storages::sqlite::OperationType::kReadWrite,
        "INSERT INTO test VALUES (?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        expected_test_row
    )) << "Try to bind values of different types";

    TestRow actual_test_row;
    UEXPECT_NO_THROW(
        (actual_test_row =
             client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test").AsSingleRow<TestRow>())
    ) << "Try to extract values of different types";

    EXPECT_EQ(actual_test_row, expected_test_row) << "Input and output rows must coincide";
}

UTEST_F(SQLiteTypesStringViewTest, BindExtract) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient());
    std::string expected_str{"forty two"};
    std::string_view expected_str_view = expected_str;
    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (?)", expected_str_view)
    ) << "Try to bind std::string_view";

    std::string actual_str;
    UEXPECT_NO_THROW(
        (actual_str = client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test")
                          .AsSingleField<std::string>())
    ) << "Try to extract std::string";

    EXPECT_EQ(actual_str, expected_str) << "Input and output std::string must coincide";
}

UTEST_F(SQLiteTypesTest, BindExtractOptionalNull) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient());
    OptionalTestRow expected_test_row{};
    UEXPECT_NO_THROW(client->ExecuteDecompose(
        storages::sqlite::OperationType::kReadWrite,
        "INSERT INTO test VALUES (?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        expected_test_row
    )) << "Try to bind values of different types";

    OptionalTestRow actual_test_row;
    UEXPECT_NO_THROW(
        (actual_test_row = client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test")
                               .AsSingleRow<OptionalTestRow>())
    ) << "Try to extract values of different types";

    EXPECT_EQ(actual_test_row, expected_test_row) << "Input and output rows must coincide";
}

UTEST_F(SQLiteTypesTest, BindExtractOptionalWithValue) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient());
    OptionalTestRow expected_test_row{
        42,
        42,
        42,
        42,
        42,
        42,
        42,
        42,
        42.42,
        42.42,
        "forty two",
        std::vector<uint8_t>{'f', 'o', 'r', 't', 'y', ' ', 't', 'w', 'o'},
        true,
        std::chrono::system_clock::now(),
        utils::generators::GenerateBoostUuid(),
        decimal64::Decimal<4, decimal64::HalfEvenRoundPolicy>{42},
        formats::json::FromString(R"({
        "key1": 42,
        "key2": {"key3":"val"}
      })")};
    UEXPECT_NO_THROW(client->ExecuteDecompose(
        storages::sqlite::OperationType::kReadWrite,
        "INSERT INTO test VALUES (?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        expected_test_row
    )) << "Try to bind values of different types";

    OptionalTestRow actual_test_row;
    UEXPECT_NO_THROW(
        (actual_test_row = client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test")
                               .AsSingleRow<OptionalTestRow>())
    ) << "Try to extract values of different types";

    EXPECT_EQ(actual_test_row, expected_test_row) << "Input and output rows must coincide";
}

UTEST_F(SQLiteTypesMismatchTest, BindExtractMismatch) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient());
    MismatchTypesTestRow expected_input_test_row{};
    MismatchTypesTestRowCorrect expected_correct_test_row{};
    UEXPECT_NO_THROW(client->ExecuteDecompose(
        storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (?, ?)", expected_input_test_row
    )) << "Try to bind values of different types";

    {
        // Extract in input order
        MismatchTypesTestRow actual_test_row;
        UEXPECT_NO_THROW(
            (actual_test_row = client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test")
                                   .AsSingleRow<MismatchTypesTestRow>())
        ) << "Try to extract values of different types";

        EXPECT_EQ(actual_test_row, expected_input_test_row) << "Input and output rows must coincide";
    }

    {
        // Extract in correct order
        MismatchTypesTestRowCorrect actual_test_row;
        UEXPECT_NO_THROW(
            (actual_test_row = client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test")
                                   .AsSingleRow<MismatchTypesTestRowCorrect>())
        ) << "Try to extract values of different types";

        EXPECT_EQ(actual_test_row, expected_correct_test_row) << "Input and output rows must coincide";
    }
}

UTEST_F(SQLiteTypesStrictMismatchTest, BindExtractMismatch) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient());
    MismatchTypesTestRow expected_test_row{};
    UEXPECT_THROW(
        client->ExecuteDecompose(
            storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (?, ?)", expected_test_row
        ),
        SQLiteException
    ) << "With STRICT SQLite will be check types";
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END

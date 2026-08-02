#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <sqlext.h>

#include <storages/odbc/detail/result_chunk.hpp>
#include <storages/odbc/detail/result_wrapper.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

namespace {

ResultSet MakeResult(std::vector<detail::ResultWrapper::Column> columns, std::vector<detail::ResultWrapper::Row> rows) {
    return ResultSet{std::make_shared<detail::ResultWrapper>(std::move(columns), std::move(rows), 0)};
}

ResultSet MakeStringResult(SQLSMALLINT type, std::optional<std::string> value) {
    detail::ResultWrapper::Cell cell;
    if (value) {
        cell.value = detail::ResultWrapper::Cell::Value{std::move(*value)};
    }
    return MakeResult({{"value", type, 0, 0}}, {{{std::move(cell)}}});
}

struct MappedRow final {
    std::int32_t id;
    std::optional<std::string> name;

    bool operator==(const MappedRow&) const = default;
};

/// [ODBC typed result mapping]
struct User final {
    std::int32_t id;
    std::string name;
    std::optional<std::int64_t> score;
};

[[maybe_unused]] void TypedResultExample(const ResultSet& result, const ResultSet& count_result) {
    const auto users = result.AsContainer<std::vector<User>>();
    const auto count = count_result.AsSingleRow<std::int64_t>();
    const auto maybe_user = result.AsOptionalSingleRow<User>();
    static_cast<void>(users);
    static_cast<void>(count);
    static_cast<void>(maybe_user);
}
/// [ODBC typed result mapping]

class PrivateMembers final {
    [[maybe_unused]]
    int value_{0};
};

union UnionValue {
    int value;
};

struct BaseValue {
    int base;
};

struct DerivedValue : BaseValue {
    int derived;
};

struct ReferenceValue {
    int& value;
};

struct EmptyValue {};

struct NestedValue {
    MappedRow nested;
};

template <typename Container>
concept CanAsContainer = requires(const ResultSet& result) { result.template AsContainer<Container>(); };

template <typename T>
concept CanAsSingleRow = requires(const ResultSet& result) { result.template AsSingleRow<T>(); };

static_assert(impl::kIsResultAggregate<MappedRow>);
static_assert(!impl::kIsResultAggregate<int>);
static_assert(!impl::kIsResultAggregate<PrivateMembers>);
static_assert(!impl::kIsResultAggregate<UnionValue>);
static_assert(!impl::kIsResultAggregate<DerivedValue>);
static_assert(!impl::kIsResultAggregate<ReferenceValue>);
static_assert(!impl::kIsResultAggregate<EmptyValue>);
static_assert(!impl::kIsResultAggregate<NestedValue>);
static_assert(CanAsContainer<std::vector<MappedRow>>);
static_assert(CanAsContainer<std::list<std::int32_t>>);
static_assert(!CanAsContainer<std::string>);
static_assert(!CanAsContainer<std::array<std::int32_t, 1>>);
static_assert(!CanAsContainer<std::span<std::int32_t>>);
static_assert(CanAsSingleRow<MappedRow>);
static_assert(!CanAsSingleRow<PrivateMembers>);
static_assert(!CanAsSingleRow<UnionValue>);
static_assert(!CanAsSingleRow<DerivedValue>);
static_assert(!CanAsSingleRow<ReferenceValue>);
static_assert(!CanAsSingleRow<NestedValue>);

}  // namespace

TEST(OdbcPortableTypes, ValidatesDateTimeAndTimestamp) {
    EXPECT_EQ(Date(2000, 2, 29).ToString(), "2000-02-29");
    EXPECT_EQ(Date(1, 1, 1).ToString(), "0001-01-01");
    EXPECT_EQ(Date(9999, 12, 31).ToString(), "9999-12-31");
    EXPECT_THROW((Date{0, 1, 1}), std::invalid_argument);
    EXPECT_THROW((Date{1900, 2, 29}), std::invalid_argument);
    EXPECT_THROW((Date{2024, 13, 1}), std::invalid_argument);

    EXPECT_EQ(Time(23, 59, 59).ToString(), "23:59:59");
    EXPECT_THROW((Time{24, 0, 0}), std::invalid_argument);
    EXPECT_THROW((Time{0, 60, 0}), std::invalid_argument);
    EXPECT_THROW((Time{0, 0, 60}), std::invalid_argument);

    EXPECT_EQ(Timestamp(2024, 2, 29, 23, 59, 59, 123'456'789).ToString(), "2024-02-29 23:59:59.123456789");
    EXPECT_EQ(Timestamp(Date{2024, 1, 2}, Time{3, 4, 5}).ToString(), "2024-01-02 03:04:05");
    EXPECT_THROW((Timestamp{2024, 1, 1, 0, 0, 0, 1'000'000'000}), std::invalid_argument);
}

TEST(OdbcPortableTypes, DecimalHasCanonicalFixedScaleValueSemantics) {
    EXPECT_EQ((Decimal<5, 2>{"+0001.20"}), (Decimal<5, 2>{"1.20"}));
    EXPECT_EQ((Decimal<5, 2>{"-000.00"}.GetRepresentation()), "0.00");
    EXPECT_EQ((Decimal<5, 2>{"000.10"}.GetRepresentation()), "0.10");
    EXPECT_EQ(
        (Decimal<38, 0>{"99999999999999999999999999999999999999"}.GetRepresentation()),
        "99999999999999999999999999999999999999"
    );

    EXPECT_THROW((Decimal<5, 2>{"1.2"}), std::invalid_argument);
    EXPECT_THROW((Decimal<5, 2>{"1.200"}), std::invalid_argument);
    EXPECT_THROW((Decimal<5, 2>{"1e2"}), std::invalid_argument);
    EXPECT_THROW((Decimal<5, 2>{" 1.20"}), std::invalid_argument);
    EXPECT_THROW((Decimal<5, 2>{"1000.00"}), std::out_of_range);
    EXPECT_THROW((Decimal<38, 0>{"999999999999999999999999999999999999999"}), std::out_of_range);

    const impl::Parameter null_decimal{std::optional<Decimal<9, 4>>{}};
    EXPECT_TRUE(null_decimal.IsNull());
    EXPECT_EQ(null_decimal.GetType(), impl::ParameterType::kDecimal);
    EXPECT_EQ(null_decimal.Get<impl::DecimalParameter>().precision, 9);
    EXPECT_EQ(null_decimal.Get<impl::DecimalParameter>().scale, 4);
}

TEST(OdbcFieldAs, ChecksNullCategoryParsingAndAllIntegerWidths) {
    EXPECT_EQ(MakeStringResult(SQL_SMALLINT, "-128")[0][0].As<std::int8_t>(), -128);
    EXPECT_EQ(MakeStringResult(SQL_SMALLINT, "127")[0][0].As<std::int8_t>(), 127);
    EXPECT_THROW(MakeStringResult(SQL_SMALLINT, "128")[0][0].As<std::int8_t>(), ResultSetError);
    EXPECT_EQ(MakeStringResult(SQL_SMALLINT, "255")[0][0].As<std::uint8_t>(), 255);
    EXPECT_THROW(MakeStringResult(SQL_SMALLINT, "256")[0][0].As<std::uint8_t>(), ResultSetError);
    EXPECT_THROW(MakeStringResult(SQL_SMALLINT, "-1")[0][0].As<std::uint8_t>(), ResultSetError);
    EXPECT_THROW(MakeStringResult(SQL_SMALLINT, "-0")[0][0].As<std::uint8_t>(), ResultSetError);

    EXPECT_EQ(MakeStringResult(SQL_INTEGER, "-32768")[0][0].As<std::int16_t>(), -32768);
    EXPECT_EQ(MakeStringResult(SQL_INTEGER, "65535")[0][0].As<std::uint16_t>(), 65535);
    EXPECT_THROW(MakeStringResult(SQL_INTEGER, "65536")[0][0].As<std::uint16_t>(), ResultSetError);
    EXPECT_EQ(
        MakeStringResult(SQL_BIGINT, "-2147483648")[0][0].As<std::int32_t>(),
        std::numeric_limits<std::int32_t>::min()
    );
    EXPECT_EQ(
        MakeStringResult(SQL_BIGINT, "4294967295")[0][0].As<std::uint32_t>(),
        std::numeric_limits<std::uint32_t>::max()
    );
    EXPECT_THROW(MakeStringResult(SQL_BIGINT, "4294967296")[0][0].As<std::uint32_t>(), ResultSetError);
    EXPECT_EQ(
        MakeStringResult(SQL_BIGINT, "-9223372036854775808")[0][0].As<std::int64_t>(),
        std::numeric_limits<std::int64_t>::min()
    );
    EXPECT_EQ(
        MakeStringResult(SQL_BIGINT, "18446744073709551615")[0][0].As<std::uint64_t>(),
        std::numeric_limits<std::uint64_t>::max()
    );
    EXPECT_THROW(MakeStringResult(SQL_BIGINT, "18446744073709551616")[0][0].As<std::uint64_t>(), ResultSetError);
    EXPECT_THROW(MakeStringResult(SQL_INTEGER, "1x")[0][0].As<std::int32_t>(), ResultSetError);

    EXPECT_THROW(MakeStringResult(SQL_VARCHAR, "1")[0][0].As<std::int32_t>(), ResultSetError);
    EXPECT_EQ(MakeStringResult(SQL_VARCHAR, "value")[0][0].As<std::string>(), "value");
    EXPECT_FALSE(MakeStringResult(SQL_INTEGER, std::nullopt)[0][0].As<std::optional<std::int32_t>>());
    EXPECT_THROW(MakeStringResult(SQL_INTEGER, std::nullopt)[0][0].As<std::int32_t>(), ResultSetError);
}

TEST(OdbcFieldAs, PreservesBinaryAndDecimalSemantics) {
    detail::ResultWrapper::Cell empty_bytes;
    empty_bytes.value = detail::ResultWrapper::Cell::Value{Bytes{}};
    auto binary = MakeResult({{"value", SQL_VARBINARY, 0, 0}}, {{{std::move(empty_bytes)}}});
    EXPECT_TRUE(binary[0][0].As<Bytes>().IsEmpty());
    EXPECT_EQ(binary[0][0].GetString(), "");

    detail::ResultWrapper::Cell decimal_cell;
    decimal_cell.value = detail::ResultWrapper::Cell::Value{
        detail::ResultWrapper::DecimalValue{"1.20", 5, 2},
    };
    auto decimal = MakeResult({{"value", SQL_DECIMAL, 5, 2}}, {{{std::move(decimal_cell)}}});
    EXPECT_EQ((decimal[0][0].As<Decimal<5, 2>>()), (Decimal<5, 2>{"1.20"}));
    EXPECT_THROW((decimal[0][0].As<Decimal<6, 2>>()), ResultSetError);
    EXPECT_THROW((decimal[0][0].As<Decimal<5, 1>>()), ResultSetError);
}

TEST(OdbcResultChunks, HandlesKnownAndUnknownLengthsWithoutGuessing) {
    EXPECT_NO_THROW(detail::ValidateFixedResultSize(sizeof(SQL_DATE_STRUCT), sizeof(SQL_DATE_STRUCT)));
    EXPECT_NO_THROW(detail::ValidateFixedResultSize(sizeof(SQL_TIME_STRUCT), sizeof(SQL_TIME_STRUCT)));
    EXPECT_NO_THROW(detail::ValidateFixedResultSize(sizeof(SQL_TIMESTAMP_STRUCT), sizeof(SQL_TIMESTAMP_STRUCT)));
    EXPECT_NO_THROW(detail::ValidateFixedResultSize(sizeof(SQL_NUMERIC_STRUCT), sizeof(SQL_NUMERIC_STRUCT)));
    EXPECT_NO_THROW(detail::ValidateFixedResultSize(0, sizeof(SQL_NUMERIC_STRUCT)));
    EXPECT_THROW(detail::ValidateFixedResultSize(SQL_NO_TOTAL, sizeof(SQL_DATE_STRUCT)), ResultSetError);
    EXPECT_NO_THROW(detail::ValidateFixedResultSize(sizeof(SQL_TIME_STRUCT) - 1, sizeof(SQL_TIME_STRUCT)));
    EXPECT_THROW(
        detail::ValidateFixedResultSize(sizeof(SQL_TIMESTAMP_STRUCT) + 1, sizeof(SQL_TIMESTAMP_STRUCT)),
        ResultSetError
    );
    EXPECT_NO_THROW(detail::ValidateNumericMagnitude(38, 38, 0));
    EXPECT_THROW(detail::ValidateNumericMagnitude(39, 38, 0), ResultSetError);
    EXPECT_NO_THROW(detail::ValidateNumericMagnitude(1, 38, 38));
    EXPECT_THROW(detail::ValidateNumericMagnitude(2, 1, 0), ResultSetError);

    const auto exact_binary = detail::AccountResultChunk(SQL_SUCCESS, 16, false, 16, std::nullopt);
    EXPECT_EQ(exact_binary.size, 16);
    EXPECT_FALSE(exact_binary.has_more);
    const auto exact_text = detail::AccountResultChunk(SQL_SUCCESS, 15, false, 15, std::nullopt);
    EXPECT_EQ(exact_text.size, 15);
    EXPECT_FALSE(exact_text.has_more);

    const auto first = detail::AccountResultChunk(SQL_SUCCESS_WITH_INFO, 100, true, 16, std::nullopt);
    EXPECT_EQ(first.size, 16);
    EXPECT_TRUE(first.has_more);
    ASSERT_TRUE(first.known_remaining);
    EXPECT_EQ(*first.known_remaining, 100);

    const auto second = detail::AccountResultChunk(SQL_SUCCESS_WITH_INFO, 84, true, 16, first.known_remaining);
    EXPECT_EQ(second.size, 16);
    EXPECT_TRUE(second.has_more);
    EXPECT_EQ(second.known_remaining, 84);
    const auto final = detail::AccountResultChunk(SQL_SUCCESS, 4, false, 16, second.known_remaining);
    EXPECT_EQ(final.size, 4);
    EXPECT_FALSE(final.has_more);

    const auto unknown = detail::AccountResultChunk(SQL_SUCCESS_WITH_INFO, SQL_NO_TOTAL, true, 16, std::nullopt);
    EXPECT_EQ(unknown.size, 16);
    EXPECT_TRUE(unknown.has_more);
    EXPECT_FALSE(unknown.known_remaining);
    EXPECT_THROW(detail::AccountResultChunk(SQL_SUCCESS, SQL_NO_TOTAL, false, 16, std::nullopt), ResultSetError);
    EXPECT_THROW(detail::AccountResultChunk(SQL_SUCCESS_WITH_INFO, 16, true, 16, std::nullopt), ResultSetError);
    EXPECT_THROW(detail::AccountResultChunk(SQL_SUCCESS_WITH_INFO, 100, true, 16, 100), ResultSetError);
    EXPECT_THROW(detail::AccountResultChunk(SQL_SUCCESS, 17, false, 16, std::nullopt), ResultSetError);
}

TEST(OdbcResultMapping, MapsScalarsAggregatesContainersAndCardinality) {
    detail::ResultWrapper::Cell first_id;
    first_id.value = detail::ResultWrapper::Cell::Value{std::string{"1"}};
    detail::ResultWrapper::Cell first_name;
    first_name.value = detail::ResultWrapper::Cell::Value{std::string{"one"}};
    detail::ResultWrapper::Cell second_id;
    second_id.value = detail::ResultWrapper::Cell::Value{std::string{"2"}};
    detail::ResultWrapper::Cell second_name;
    auto rows = MakeResult(
        {{"id", SQL_INTEGER, 10, 0}, {"name", SQL_VARCHAR, 32, 0}},
        {{{std::move(first_id)}, {std::move(first_name)}}, {{std::move(second_id)}, {std::move(second_name)}}}
    );

    const auto values = rows.AsContainer<std::vector<MappedRow>>();
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], (MappedRow{1, "one"}));
    EXPECT_EQ(values[1], (MappedRow{2, std::nullopt}));
    EXPECT_THROW(rows.AsSingleRow<MappedRow>(), ResultSetError);
    EXPECT_THROW(rows.AsOptionalSingleRow<MappedRow>(), ResultSetError);

    auto one = MakeStringResult(SQL_INTEGER, "42");
    EXPECT_EQ(one.AsSingleRow<std::int32_t>(), 42);
    EXPECT_EQ(one.AsContainer<std::vector<std::int32_t>>(), (std::vector<std::int32_t>{42}));
    EXPECT_EQ(one.AsContainer<std::list<std::int32_t>>(), (std::list<std::int32_t>{42}));
    const auto
        present_null = MakeStringResult(SQL_INTEGER, std::nullopt).AsOptionalSingleRow<std::optional<std::int32_t>>();
    ASSERT_TRUE(present_null);
    EXPECT_FALSE(*present_null);

    auto no_rows = MakeResult({{"value", SQL_INTEGER, 10, 0}}, {});
    EXPECT_FALSE(no_rows.AsOptionalSingleRow<std::optional<std::int32_t>>());
    EXPECT_THROW(no_rows.AsSingleRow<std::int32_t>(), ResultSetError);

    detail::ResultWrapper::Cell extra_first;
    extra_first.value = detail::ResultWrapper::Cell::Value{std::string{"1"}};
    detail::ResultWrapper::Cell extra_second;
    extra_second.value = detail::ResultWrapper::Cell::Value{std::string{"2"}};
    auto extra_columns = MakeResult(
        {{"first", SQL_INTEGER, 10, 0}, {"second", SQL_INTEGER, 10, 0}},
        {{{std::move(extra_first)}, {std::move(extra_second)}}}
    );
    EXPECT_THROW(extra_columns.AsSingleRow<std::int32_t>(), ResultSetError);
    EXPECT_THROW(extra_columns.AsSingleRow<MappedRow>(), ResultSetError);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END

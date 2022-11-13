#include <userver/utest/utest.hpp>

#include <set>
#include <string>
#include <vector>

#include <userver/storages/clickhouse/io/impl/escape.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
namespace io = storages::clickhouse::io;
using clock = std::chrono::system_clock;

using QueryTester = storages::clickhouse::QueryTester;

constexpr clock::time_point kFakeNow{
    std::chrono::duration_cast<clock::duration>(
        std::chrono::nanoseconds{1546300800'123456789ULL})};

void ValidateEscaping(const std::string& source, const std::string& expected) {
  const auto escaped = io::impl::Escape(source);
  EXPECT_EQ(escaped, expected);
}

}  // namespace

TEST(EscapeString, Basic) {
  ValidateEscaping("just a text", R"('just a text')");
}

TEST(EscapeString, StringWithSingleQuote) {
  ValidateEscaping("with'", R"('with\'')");
}

TEST(EscapeString, StringWithDoubleQuote) {
  ValidateEscaping("with\"", R"('with"')");
}

TEST(EscapeString, SpecialSymbols) {
  ValidateEscaping(std::string{"\b\f\r\n\t\0\a\v\\\'", 10},
                   R"('\b\f\r\n\t\0\a\v\\\'')");
}

TEST(EscapeScalar, Basic) {
  const storages::clickhouse::Query q{"{} {} {} {} {} {} {} {}"};
  const auto formatted_query = QueryTester::WithArgs(q, uint8_t{1},  //
                                                     uint16_t{2},    //
                                                     uint32_t{3},    //
                                                     uint64_t{4},    //
                                                                     //
                                                     int8_t{5},      //
                                                     int16_t{6},     //
                                                     int32_t{7},     //
                                                     int64_t{8});

  EXPECT_EQ(formatted_query.QueryText(), "1 2 3 4 5 6 7 8");
}

TEST(EscapeVectorString, Basic) {
  const std::vector<std::string> source = {"a", "b", "c"};
  const std::string expected = R"(['a','b','c'])";
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

TEST(EscapeSetString, Basic) {
  const std::set<std::string> source = {"a", "b"};
  const std::string expected = R"(['a','b'])";
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

template <typename T>
class MyRange final {
 public:
  MyRange(std::initializer_list<T> data) : data_{data} {}

  auto begin() const { return data_.begin(); }

  auto end() const { return data_.end(); }

 private:
  std::vector<T> data_;
};

TEST(EscapeRangeInt, Basic) {
  const MyRange<int> source{1, 2, 3};
  const std::string expected = R"([1,2,3])";
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

TEST(EscapeRangeString, Basic) {
  const MyRange<std::string_view> source{"a", "b", "c"};
  const std::string expected = R"(['a','b','c'])";
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

TEST(EscapeDatetime, Basic) {
  const clock::time_point source{kFakeNow};
  const std::string expected = R"(toDateTime(1546300800))";
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

TEST(EscapeDatetime, Milli) {
  const io::DateTime64Milli source{kFakeNow};
  // precision is 10^-3
  const std::string expected = R"(toDateTime64('1546300800.123', 3))";
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

TEST(EscapeDatetime, Micro) {
  const io::DateTime64Micro source{kFakeNow};
  // precision is 10^-6
  const std::string expected = R"(toDateTime64('1546300800.123456', 6))";
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

TEST(EscapeDatetime, Nano) {
  const io::DateTime64Nano source{kFakeNow};
  // precision is 10^-9
#ifdef _LIBCPP_VERSION
  const std::string expected = R"(toDateTime64('1546300800.123456000', 9))";
#else
  const std::string expected = R"(toDateTime64('1546300800.123456789', 9))";
#endif
  const auto escaped = io::impl::Escape(source);

  EXPECT_EQ(escaped, expected);
}

TEST(EscapeQuery, ParamsCountMismatch) {
  const storages::clickhouse::Query q{"{} {} {}"};
  EXPECT_ANY_THROW(QueryTester::WithArgs(q, 1));
  EXPECT_ANY_THROW(QueryTester::WithArgs(q, 1, 2));
  EXPECT_NO_THROW(QueryTester::WithArgs(q, 1, 2, 3));
  // ideally this should throw, but oh well
  // TODO : https://st.yandex-team.ru/TAXICOMMON-5066
  EXPECT_NO_THROW(QueryTester::WithArgs(q, 1, 2, 3, 4));
}

USERVER_NAMESPACE_END

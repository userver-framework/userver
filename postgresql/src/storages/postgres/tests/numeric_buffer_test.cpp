#include <userver/utest/utest.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <string>

#include <storages/postgres/tests/test_buffers.hpp>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/numeric_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

io::detail::IntegralRepresentation ParseNumeric(const std::string& str_rep) {
    const auto buffer = io::detail::StringToNumericBuffer(str_rep);
    return io::detail::NumericBufferToInt64(pg::test::MakeFieldBuffer(buffer));
}

TEST(PostgreIONumericBuffer, Int64Limits) {
    constexpr auto kMin = std::numeric_limits<std::int64_t>::min();
    constexpr auto kMax = std::numeric_limits<std::int64_t>::max();
    struct TestCase {
        const char* text;
        io::detail::IntegralRepresentation expected;
    };
    constexpr std::array kCases{
        TestCase{"9223372036854775807", {kMax, 0}},
        TestCase{"-9223372036854775808", {kMin, 0}},
        TestCase{"922337203685477580.7", {kMax, 1}},
        TestCase{"-922337203685477580.8", {kMin, 1}},
        TestCase{"-9.223372036854775808", {kMin, 18}},
        TestCase{"92.23372036854775807", {kMax, 17}},
    };

    for (const auto& test_case : kCases) {
        SCOPED_TRACE(test_case.text);
        const auto expected_buffer = io::detail::StringToNumericBuffer(test_case.text);
        const auto parsed = io::detail::NumericBufferToInt64(pg::test::MakeFieldBuffer(expected_buffer));
        EXPECT_EQ(parsed.value, test_case.expected.value);
        EXPECT_EQ(parsed.fractional_digit_count, test_case.expected.fractional_digit_count);

        const auto formatted = io::detail::Int64ToNumericBuffer(test_case.expected);
        EXPECT_EQ(formatted, expected_buffer);
    }

    UEXPECT_THROW(ParseNumeric("9223372036854775808"), pg::NumericOverflow);
    UEXPECT_THROW(ParseNumeric("-9223372036854775809"), pg::NumericOverflow);
}

// 10^20 exceeds int64 by a multiple of 2^64 that a wraparound check does not
// detect, the value used to be returned as 7766279631452241920.
TEST(PostgreIONumericBuffer, IntegralPartOverflow) {
    UEXPECT_THROW(ParseNumeric("100000000000000000000"), pg::NumericOverflow);
    UEXPECT_THROW(ParseNumeric("-100000000000000000000"), pg::NumericOverflow);
}

TEST(PostgreIONumericBuffer, FractionalPartOverflow) {
    UEXPECT_THROW(ParseNumeric("10000000000000000.0001"), pg::NumericOverflow);
    UEXPECT_THROW(ParseNumeric("922337203685477580.8"), pg::NumericOverflow);
    UEXPECT_THROW(ParseNumeric("-922337203685477580.9"), pg::NumericOverflow);
}

}  // namespace

USERVER_NAMESPACE_END

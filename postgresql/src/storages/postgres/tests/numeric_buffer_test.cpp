#include <userver/utest/utest.hpp>

#include <cstdint>
#include <limits>
#include <string>

#include <storages/postgres/tests/test_buffers.hpp>

#include <userver/decimal64/decimal64.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/decimal64.hpp>
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
    const auto max = ParseNumeric("9223372036854775807");
    EXPECT_EQ(max.value, std::numeric_limits<std::int64_t>::max());
    EXPECT_EQ(max.fractional_digit_count, 0);

    const auto min = ParseNumeric("-9223372036854775807");
    EXPECT_EQ(min.value, -std::numeric_limits<std::int64_t>::max());

    UEXPECT_THROW(ParseNumeric("9223372036854775808"), pg::NumericOverflow);
}

// 10^20 exceeds int64 by a multiple of 2^64 that a wraparound check does not
// detect, the value used to be returned as 7766279631452241920.
TEST(PostgreIONumericBuffer, IntegralPartOverflow) {
    UEXPECT_THROW(ParseNumeric("100000000000000000000"), pg::NumericOverflow);
    UEXPECT_THROW(ParseNumeric("-100000000000000000000"), pg::NumericOverflow);
}

TEST(PostgreIONumericBuffer, FractionalPartOverflow) {
    UEXPECT_THROW(ParseNumeric("10000000000000000.0001"), pg::NumericOverflow);
}

TEST(PostgreIONumericBuffer, Decimal64Overflow) {
    const auto buffer = io::detail::StringToNumericBuffer("100000000000000000000");
    decimal64::Decimal<0> value;
    UEXPECT_THROW(io::ReadBuffer(pg::test::MakeFieldBuffer(buffer), value), pg::NumericOverflow);
}

}  // namespace

USERVER_NAMESPACE_END

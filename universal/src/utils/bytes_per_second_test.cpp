#include <userver/utils/bytes_per_second.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct DataT {
    const char* const name;
    const char* const data;
    const unsigned long long ethalon;
};

inline std::string PrintToString(const DataT& d) { return d.name; }

using TestData = std::initializer_list<DataT>;

}  // namespace

////////////////////////////////////////////////////////////////////////////////

class StringToBytesPerSecond : public ::testing::TestWithParam<DataT> {};

INSTANTIATE_TEST_SUITE_P(
    /*no prefix*/,
    StringToBytesPerSecond,
    ::testing::ValuesIn(TestData{
        {.name = "bytes", .data = "103B/s", .ethalon = 103},
        {.name = "megaBytes", .data = "3MiB/s", .ethalon = 3 * 1024 * 1024},
        {.name = "mb", .data = "3mb/s", .ethalon = 3 * 1000 * 1000},
    }),
    ::testing::PrintToStringParamName()
);

TEST_P(StringToBytesPerSecond, Basic) {
    const auto p = GetParam();
    auto val = utils::StringToBytesPerSecond(p.data);
    EXPECT_EQ(static_cast<std::size_t>(val), p.ethalon);
}

USERVER_NAMESPACE_END

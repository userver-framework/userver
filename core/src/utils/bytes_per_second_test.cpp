#include <userver/utils/bytes_per_second.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct data_t {
  const char* const name;
  const char* const data;
  const unsigned long long ethalon;
};

inline std::string PrintToString(const data_t& d) { return d.name; }

using TestData = std::initializer_list<data_t>;

}  // namespace

////////////////////////////////////////////////////////////////////////////////

class StringToBytesPerSecond : public ::testing::TestWithParam<data_t> {};

INSTANTIATE_TEST_SUITE_P(
    /*no prefix*/, StringToBytesPerSecond,
    ::testing::ValuesIn(TestData{
        {"bytes", "103B/s", 103},
        {"megaBytes", "3MiB/s", 3 * 1024 * 1024},
        {"mb", "3mb/s", 3 * 1000 * 1000},
    }),
    ::testing::PrintToStringParamName());

TEST_P(StringToBytesPerSecond, Basic) {
  const auto p = GetParam();
  auto val = utils::StringToBytesPerSecond(p.data);
  EXPECT_EQ(static_cast<std::size_t>(val), p.ethalon);
}

USERVER_NAMESPACE_END

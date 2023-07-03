#include <gtest/gtest.h>

#include <userver/logging/format.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
struct TestArguments {
  std::string format_str;
  logging::Format format_enum;
};
}  // namespace

class FormatFromStringFixture : public ::testing::TestWithParam<TestArguments> {
};

INSTANTIATE_TEST_SUITE_P(
    FormatFromStringTests, FormatFromStringFixture,
    ::testing::Values(TestArguments{"tskv", logging::Format::kTskv},
                      TestArguments{"ltsv", logging::Format::kLtsv},
                      TestArguments{"raw", logging::Format::kRaw},
                      TestArguments{"json", logging::Format::kJson}));

TEST_P(FormatFromStringFixture, ExpectedFormats) {
  const TestArguments args = GetParam();

  const auto format_enum = logging::FormatFromString(args.format_str);
  ASSERT_EQ(args.format_enum, format_enum);
}

USERVER_NAMESPACE_END
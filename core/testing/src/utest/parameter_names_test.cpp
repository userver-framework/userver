#include <userver/utest/parameter_names.hpp>

#include <ostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

/// [PrintTestName Example Usage - Singly-Parameterized Test]
// Declare a structure to keep input parameters for a test.
struct TestParams {
  int id;
  std::string test_name;
};

// Define a class for running tests which get parameterized with the structure
// defined above.
class ParametrizedTest : public testing::TestWithParam<TestParams> {};

// Create a list of parameters' values, each item corresponding to a separate
// test run with specific input.
const std::vector<TestParams> kTestParams = {
    {1, "First"},
    {2, "Second"},
    {3, "Third"},
};

TEST_P(ParametrizedTest, BasicTest) {
  const auto& param = GetParam();

  const utest::PrintTestName test_name_printer;
  const testing::TestParamInfo<TestParams> param_info(param, /* index */ 0);

  EXPECT_EQ(test_name_printer(param_info), param.test_name);
}

// Pass utest::PrintTestName() as the last argument for
// INSTANTIATE_TEST_SUITE_P macro.
INSTANTIATE_TEST_SUITE_P(/* no prefix */, ParametrizedTest,
                         testing::ValuesIn(kTestParams),
                         utest::PrintTestName());
/// [PrintTestName Example Usage - Singly-Parameterized Test]

/// [PrintTestName Example Usage - Override PrintTo]
struct AnotherTestParams {
  int id;
  std::string name;
};

void PrintTo(const AnotherTestParams& params, std::ostream* output_stream) {
  *output_stream << "Custom" << params.name;
}

class AnotherParametrizedTest
    : public testing::TestWithParam<AnotherTestParams> {};

const std::vector<AnotherTestParams> kAnotherTestParams = {
    {1, "First"},
    {2, "Second"},
};

TEST_P(AnotherParametrizedTest, BasicTest) {
  const auto& param = GetParam();

  const utest::PrintTestName test_name_printer;
  const testing::TestParamInfo<AnotherTestParams> param_info(param,
                                                             /* index */ 0);

  EXPECT_EQ(test_name_printer(param_info), "Custom" + param.name);
}

INSTANTIATE_TEST_SUITE_P(/* no prefix */, AnotherParametrizedTest,
                         testing::ValuesIn(kAnotherTestParams),
                         utest::PrintTestName());
/// [PrintTestName Example Usage - Override PrintTo]

/// [PrintTestName Example Usage - Doubly-Parameterized Test]
// Define a class for running tests which get parameterized with the structures
// defined above.
class DoublyParametrizedTest
    : public testing::TestWithParam<std::tuple<TestParams, AnotherTestParams>> {
};

TEST_P(DoublyParametrizedTest, BasicTest) {
  const auto& param = GetParam();

  const utest::PrintTestName test_name_printer;
  const testing::TestParamInfo<std::tuple<TestParams, AnotherTestParams>>
      param_info(param, /* index */ 0);

  const auto expected_test_name =
      std::get<0>(param).test_name + "_Custom" + std::get<1>(param).name;
  EXPECT_EQ(test_name_printer(param_info), expected_test_name);
}

// Instantiate your test with the list of possible inputs defined earlier.
// Pass utest::PrintTestName() as the last argument for
// INSTANTIATE_TEST_SUITE_P macro.
INSTANTIATE_TEST_SUITE_P(
    /* no prefix */, DoublyParametrizedTest,
    testing::Combine(testing::ValuesIn(kTestParams),
                     testing::ValuesIn(kAnotherTestParams)),
    utest::PrintTestName());
/// [PrintTestName Example Usage - Doubly-Parameterized Test]

}  // namespace

USERVER_NAMESPACE_END

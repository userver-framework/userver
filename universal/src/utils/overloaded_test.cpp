#include <userver/utils/overloaded.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

using TestData = std::variant<std::string, std::vector<int>>;

class OverloadedTestFixture : public ::testing::TestWithParam<TestData> {};

}  // namespace

INSTANTIATE_TEST_SUITE_P(/*no prefix*/, OverloadedTestFixture,
                         ::testing::Values(TestData{"Hello, Userver!"},
                                           TestData{std::vector<int>{1, 2}}));

TEST_P(OverloadedTestFixture, StdVisit) {
  TestData data = GetParam();

  std::visit(utils::Overloaded{
                 [](std::string& data) {
                   EXPECT_EQ(data, "Hello, Userver!");
                   data = "Farewell, Userver!";
                 },
                 [](std::vector<int>& data) {
                   EXPECT_EQ(data, (std::vector<int>{1, 2}));
                   data.push_back(3);
                 },
             },
             data);

  std::visit(utils::Overloaded{
                 [](const std::string& data) {
                   EXPECT_EQ(data, "Farewell, Userver!");
                 },
                 [](const std::vector<int>& data) {
                   EXPECT_EQ(data, (std::vector<int>{1, 2, 3}));
                 },
             },
             std::as_const(data));
}

TEST_P(OverloadedTestFixture, UtilsVisit) {
  TestData data = GetParam();

  utils::Visit(
      data,
      [](std::string& data) {
        EXPECT_EQ(data, "Hello, Userver!");
        data = "Farewell, Userver!";
      },
      [](std::vector<int>& data) {
        EXPECT_EQ(data, (std::vector<int>{1, 2}));
        data.push_back(3);
      });

  utils::Visit(
      data,
      [](const std::string& data) { EXPECT_EQ(data, "Farewell, Userver!"); },
      [](const std::vector<int>& data) {
        EXPECT_EQ(data, (std::vector<int>{1, 2, 3}));
      });

  utils::Visit(
      std::move(data),
      [](std::string&& data) { EXPECT_EQ(data, "Farewell, Userver!"); },
      [](std::vector<int>&& data) {
        EXPECT_EQ(data, (std::vector<int>{1, 2, 3}));
      });
}

TEST(OverloadedTest, ReturnReference) {
  struct Real {
    double Value;
  };

  struct Complex {
    double Real;
    double Imag;
  };

  std::variant<Real, Complex> var_real = Real{1.0};
  std::variant<Real, Complex> var_complex = Complex{2.0, 3.0};

  std::visit(utils::Overloaded{
                 [](Real& real) -> double& { return real.Value; },
                 [](Complex& complex) -> double& { return complex.Real; },
             },
             var_real) = 2.0;
  EXPECT_EQ(std::get<Real>(var_real).Value, 2.0);

  utils::Visit(
      var_real, [](Real& realvalue) -> double& { return realvalue.Value; },
      [](Complex& complex) -> double& { return complex.Real; }) = 3.0;
  EXPECT_EQ(std::get<Real>(var_real).Value, 3.0);

  std::visit(utils::Overloaded{
                 [](Real& real) -> double& { return real.Value; },
                 [](Complex& complex) -> double& { return complex.Real; },
             },
             var_complex) = 4.0;
  EXPECT_EQ(std::get<Complex>(var_complex).Real, 4.0);

  utils::Visit(
      var_complex, [](Real& real) -> double& { return real.Value; },
      [](Complex& complex) -> double& { return complex.Real; }) = 5.0;
  EXPECT_EQ(std::get<Complex>(var_complex).Real, 5.0);
}

TEST(OverloadedTest, NoCopying) {
  struct Complex {
    double Real;
    double Imag;

    Complex() : Real{}, Imag{} {}

    Complex(double r, double i) : Real{r}, Imag{i} {}

    Complex(const Complex&) = delete;
    Complex& operator=(const Complex&) = delete;
  };

  std::variant<Complex> var_complex;
  var_complex.emplace<Complex>(2.0, 3.0);

  utils::Visit(std::move(var_complex), [](Complex&& complex) {
    EXPECT_EQ(complex.Real, 2.0);
    EXPECT_EQ(complex.Imag, 3.0);
  });
}

USERVER_NAMESPACE_END

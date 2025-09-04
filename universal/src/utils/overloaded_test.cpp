#include <userver/utils/overloaded.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

using TestData = std::variant<std::string, std::vector<int>>;

class OverloadedTestFixture : public ::testing::TestWithParam<TestData> {};

}  // namespace

INSTANTIATE_TEST_SUITE_P(
    /*no prefix*/,
    OverloadedTestFixture,
    ::testing::Values(TestData{"Hello, Userver!"}, TestData{std::vector<int>{1, 2}})
);

TEST_P(OverloadedTestFixture, StdVisit) {
    TestData data = GetParam();

    std::visit(
        utils::Overloaded{
            [](std::string& data) {
                EXPECT_EQ(data, "Hello, Userver!");
                data = "Farewell, Userver!";
            },
            [](std::vector<int>& data) {
                EXPECT_EQ(data, (std::vector<int>{1, 2}));
                data.push_back(3);
            },
        },
        data
    );

    std::visit(
        utils::Overloaded{
            [](const std::string& data) { EXPECT_EQ(data, "Farewell, Userver!"); },
            [](const std::vector<int>& data) {
                EXPECT_EQ(data, (std::vector<int>{1, 2, 3}));
            },
        },
        std::as_const(data)
    );
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
        }
    );

    utils::Visit(
        data,
        [](const std::string& data) { EXPECT_EQ(data, "Farewell, Userver!"); },
        [](const std::vector<int>& data) {
            EXPECT_EQ(data, (std::vector<int>{1, 2, 3}));
        }
    );

    utils::Visit(
        std::move(data),
        [](std::string&& data) { EXPECT_EQ(data, "Farewell, Userver!"); },
        [](std::vector<int>&& data) {
            EXPECT_EQ(data, (std::vector<int>{1, 2, 3}));
        }
    );
}

TEST(OverloadedTest, ReturnReference) {
    struct Real {
        double value;
    };

    struct Complex {
        double real;
        double imag;
    };

    std::variant<Real, Complex> var_real = Real{1.0};
    std::variant<Real, Complex> var_complex = Complex{2.0, 3.0};

    std::visit(
        utils::Overloaded{
            [](Real& real) -> double& { return real.value; },
            [](Complex& complex) -> double& { return complex.real; },
        },
        var_real
    ) = 2.0;
    EXPECT_EQ(std::get<Real>(var_real).value, 2.0);

    utils::Visit(
        var_real,
        [](Real& realvalue) -> double& { return realvalue.value; },
        [](Complex& complex) -> double& { return complex.real; }
    ) = 3.0;
    EXPECT_EQ(std::get<Real>(var_real).value, 3.0);

    std::visit(
        utils::Overloaded{
            [](Real& real) -> double& { return real.value; },
            [](Complex& complex) -> double& { return complex.real; },
        },
        var_complex
    ) = 4.0;
    EXPECT_EQ(std::get<Complex>(var_complex).real, 4.0);

    utils::Visit(
        var_complex,
        [](Real& real) -> double& { return real.value; },
        [](Complex& complex) -> double& { return complex.real; }
    ) = 5.0;
    EXPECT_EQ(std::get<Complex>(var_complex).real, 5.0);
}

TEST(OverloadedTest, NoCopying) {
    struct Complex {
        double real;
        double imag;

        Complex() : real{}, imag{} {}

        Complex(double r, double i) : real{r}, imag{i} {}

        Complex(const Complex&) = delete;
        Complex& operator=(const Complex&) = delete;
    };

    std::variant<Complex> var_complex;
    var_complex.emplace<Complex>(2.0, 3.0);

    utils::Visit(std::move(var_complex), [](Complex&& complex) {
        EXPECT_EQ(complex.real, 2.0);
        EXPECT_EQ(complex.imag, 3.0);
    });
}

USERVER_NAMESPACE_END

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
            [](const std::vector<int>& data) { EXPECT_EQ(data, (std::vector<int>{1, 2, 3})); },
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
        [](const std::vector<int>& data) { EXPECT_EQ(data, (std::vector<int>{1, 2, 3})); }
    );

    utils::Visit(
        std::move(data),
        [](std::string&& data) { EXPECT_EQ(data, "Farewell, Userver!"); },
        [](std::vector<int>&& data) { EXPECT_EQ(data, (std::vector<int>{1, 2, 3})); }
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

    std::variant<Real, Complex> var_real = Real{.value = 1.0};
    std::variant<Real, Complex> var_complex = Complex{.real = 2.0, .imag = 3.0};

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

        Complex()
            : real{},
              imag{}
        {}

        Complex(double r, double i)
            : real{r},
              imag{i}
        {}

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

TEST(OverloadedTest, VisitExplicitReturnType) {
    std::variant<int, double> var = 1.5;
    const auto visit_int = [](int value) { return value; };
    const auto visit_double = [](double value) { return value; };

    static_assert(std::is_same_v<decltype(utils::Visit<double>(var, visit_int, visit_double)), double>);
    EXPECT_DOUBLE_EQ(utils::Visit<double>(var, visit_int, visit_double), 1.5);
}

TEST(OverloadedTest, VisitExplicitReturnTypeSameAsFirstAlternative) {
    std::variant<int, double> var = 7;

    const auto
        lvalue_result = utils::Visit<int>(var, [](int&) -> short { return 1; }, [](double&) -> long { return 2; });
    EXPECT_EQ(lvalue_result, 1);

    const auto const_lvalue_result = utils::Visit<
        int>(std::as_const(var), [](const int&) -> short { return 3; }, [](const double&) -> long { return 4; });
    EXPECT_EQ(const_lvalue_result, 3);

    const auto rvalue_result = utils::Visit<
        int>(std::move(var), [](int&&) -> short { return 5; }, [](double&&) -> long { return 6; });
    EXPECT_EQ(rvalue_result, 5);
}

TEST(OverloadedTest, VisitVoidDiscardsResult) {
    std::variant<int, double> var = 7;
    bool int_visited = false;
    const auto visit_int = [&int_visited](int value) {
        int_visited = true;
        return value;
    };
    const auto visit_double = [](double) { return std::string{"double"}; };

    static_assert(std::is_same_v<decltype(utils::Visit<void>(var, visit_int, visit_double)), void>);
    utils::Visit<void>(var, visit_int, visit_double);

    EXPECT_TRUE(int_visited);
}

USERVER_NAMESPACE_END

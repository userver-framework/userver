#include <userver/utils/invoke_with_expected.hpp>

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

class InvocationError : public std::runtime_error {
public:
    InvocationError() : std::runtime_error("invocation failed") {}
};

TEST(InvokeWithExpected, InvokesOnceAndCopiesReferenceResult) {
    int value = 42;
    int calls = 0;
    const auto result = utils::InvokeWithExpected([&value, &calls]() -> int& {
        ++calls;
        return value;
    });
    static_assert(std::is_same_v<std::remove_cv_t<decltype(result)>, utils::expected<int, std::exception_ptr>>);
    value = 100;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(calls, 1);
}

TEST(InvokeWithExpected, SupportsVoid) {
    int calls = 0;
    const auto result = utils::InvokeWithExpected([&calls] { ++calls; });
    static_assert(std::is_same_v<std::remove_cv_t<decltype(result)>, utils::expected<void, std::exception_ptr>>);
    EXPECT_NO_THROW(result.value());
    EXPECT_EQ(calls, 1);
}

TEST(InvokeWithExpected, PreservesCallableValueCategoryAndMoveOnlyResults) {
    struct MoveOnlyCallable {
        std::unique_ptr<int> value;
        std::unique_ptr<int> operator()() && { return std::move(value); }
    };
    const auto result = utils::InvokeWithExpected(MoveOnlyCallable{.value = std::make_unique<int>(42)});
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result.value());
    EXPECT_EQ(*result.value(), 42);
}

TEST(InvokeWithExpected, DecaysArrayAndFunctionReferences) {
    const char text[] = "hello";
    const auto array_result = utils::InvokeWithExpected([&text]() -> const char(&)[6] { return text; });
    static_assert(std::is_same_v<
                  std::remove_cv_t<decltype(array_result)>,
                  utils::expected<const char*, std::exception_ptr>>);
    EXPECT_EQ(array_result.value(), text);

    int (*const function)() = [] { return 42; };
    const auto function_result = utils::InvokeWithExpected([function]() -> int (&)() { return *function; });
    static_assert(std::is_same_v<
                  std::remove_cv_t<decltype(function_result)>,
                  utils::expected<int (*)(), std::exception_ptr>>);
    EXPECT_EQ(function_result.value()(), 42);
}

TEST(InvokeWithExpected, PreservesExceptionTypeForValueAndVoid) {
    const auto result = utils::InvokeWithExpected([]() -> int { throw InvocationError{}; });
    ASSERT_FALSE(result.has_value());
    EXPECT_THROW(std::rethrow_exception(result.error()), InvocationError);

    const auto void_result = utils::InvokeWithExpected([] { throw InvocationError{}; });
    ASSERT_FALSE(void_result.has_value());
    EXPECT_THROW(std::rethrow_exception(void_result.error()), InvocationError);
}

TEST(InvokeWithExpected, PropagatesNonStandardExceptions) {
    // Intentionally throw non-standard exceptions to verify that the helper propagates them.
    // NOLINTNEXTLINE(hicpp-exception-baseclass)
    EXPECT_THROW([[maybe_unused]] const auto result = utils::InvokeWithExpected([]() -> int { throw 42; }), int);
    // NOLINTNEXTLINE(hicpp-exception-baseclass)
    EXPECT_THROW([[maybe_unused]] const auto result = utils::InvokeWithExpected([] { throw 42; }), int);
}

}  // namespace

USERVER_NAMESPACE_END

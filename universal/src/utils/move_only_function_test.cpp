#include <userver/utils/move_only_function.hpp>

#include <string>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(MoveOnlyFunction, Smoke) {
    auto prefix = std::make_unique<std::string>("prefix ");
    auto functor = [prefix = std::move(prefix)](std::string_view suffix) { return *prefix + std::string{suffix}; };
    utils::move_only_function<std::string(std::string_view)> erased_functor{std::move(functor)};
    EXPECT_EQ(erased_functor("foo"), "prefix foo");
}

TEST(MoveOnlyFunction, ConstCorrectness) {
    [[maybe_unused]] auto functor = [i = 42]() mutable { return ++i; };

    using MutableFunctor = utils::move_only_function<int()>;
    static_assert(std::is_constructible_v<MutableFunctor, decltype(functor)>);
    static_assert(std::is_assignable_v<MutableFunctor, decltype(functor)>);

    using ConstFunctor = utils::move_only_function<int() const>;
    static_assert(!std::is_constructible_v<ConstFunctor, decltype(functor)>);
    static_assert(!std::is_assignable_v<ConstFunctor, decltype(functor)>);
}

USERVER_NAMESPACE_END

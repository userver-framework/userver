#include <userver/utils/forward_like.hpp>

#include <utility>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(ForwardLike, Basic) {
    char lvalue{};
    const char const_lvalue{};

    EXPECT_TRUE((std::is_same_v<char&&, decltype(utils::ForwardLike<int>(char{}))>));
    EXPECT_TRUE((std::is_same_v<char&&, decltype(utils::ForwardLike<int>(lvalue))>));
    EXPECT_TRUE((std::is_same_v<const char&&, decltype(utils::ForwardLike<int>(const_lvalue))>));

    EXPECT_TRUE((std::is_same_v<char&&, decltype(utils::ForwardLike<int&&>(char{}))>));
    EXPECT_TRUE((std::is_same_v<char&&, decltype(utils::ForwardLike<int&&>(lvalue))>));
    EXPECT_TRUE((std::is_same_v<const char&&, decltype(utils::ForwardLike<int&&>(const_lvalue))>));

    EXPECT_TRUE((std::is_same_v<char&, decltype(utils::ForwardLike<int&>(char{}))>));
    EXPECT_TRUE((std::is_same_v<char&, decltype(utils::ForwardLike<int&>(lvalue))>));
    EXPECT_TRUE((std::is_same_v<const char&, decltype(utils::ForwardLike<int&>(const_lvalue))>));

    EXPECT_TRUE((std::is_same_v<const char&, decltype(utils::ForwardLike<const int&>(char{}))>));
    EXPECT_TRUE((std::is_same_v<const char&, decltype(utils::ForwardLike<const int&>(lvalue))>));
    EXPECT_TRUE((std::is_same_v<const char&, decltype(utils::ForwardLike<const int&>(const_lvalue))>));
}

#if defined(__cpp_lib_forward_like) || defined(ARCADIA_ROOT)

namespace {

template <typename T, typename U>
void TestsTwoForwards(U&& x) {
    static_assert(std::is_same_v<
                  decltype(std::forward_like<T>(std::forward<U>(x))),
                  decltype(utils::ForwardLike<T>(std::forward<U>(x)))>);
};

}  // namespace

TEST(ForwardLike, StdForwardLike) {
    char lvalue{};
    const char const_lvalue{};

    TestsTwoForwards<int>(char{});
    TestsTwoForwards<int>(lvalue);
    TestsTwoForwards<int>(const_lvalue);

    TestsTwoForwards<int&&>(char{});
    TestsTwoForwards<int&&>(lvalue);
    TestsTwoForwards<int&&>(const_lvalue);

    TestsTwoForwards<int&>(char{});
    TestsTwoForwards<int&>(lvalue);
    TestsTwoForwards<int&>(const_lvalue);

    TestsTwoForwards<const int&>(char{});
    TestsTwoForwards<const int&>(lvalue);
    TestsTwoForwards<const int&>(const_lvalue);
}

#endif

USERVER_NAMESPACE_END

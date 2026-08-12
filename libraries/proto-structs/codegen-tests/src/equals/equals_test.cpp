#include <concepts>

#include <gtest/gtest.h>

#include <equals/base.pb.h>
#include <equals/base.structs.usrv.pb.hpp>

namespace ss = equals::structs;

USERVER_NAMESPACE_BEGIN

TEST(Equals, Simple) {
    ss::TestEqualOperator left;
    ss::TestEqualOperator right;

    left.some_integer = 1;
    right.some_integer = 2;
    EXPECT_NE(left, right);
    right.some_integer = 1;
    EXPECT_EQ(left, right);

    left.str = "1";
    right.str = "2";
    EXPECT_NE(left, right);
    right.str = "1";
    EXPECT_EQ(left, right);

    left.arr = {"1", "2"};
    right.arr = {"1"};
    EXPECT_NE(left, right);
    right.arr.push_back("2");
    EXPECT_EQ(left, right);

    left.something.set_foo(1);
    EXPECT_NE(left, right);

    ss::Recursive a{};
    ss::Recursive b{};
    EXPECT_EQ(a, b);
}

USERVER_NAMESPACE_END

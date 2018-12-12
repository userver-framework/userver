#include <gtest/gtest.h>

#include <utest/utest.hpp>
#include <utils/demangle.hpp>

TEST(Demangle, Int) { EXPECT_EQ("int", utils::GetTypeName(typeid(int))); }

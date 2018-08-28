#include <gtest/gtest.h>

#include <main_test.hpp>
#include <utils/demangle.hpp>

TEST(Demangle, Int) { EXPECT_EQ("int", utils::GetTypeName(typeid(int))); }

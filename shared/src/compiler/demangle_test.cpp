#include <gtest/gtest.h>

#include <compiler/demangle.hpp>

TEST(Demangle, Int) { EXPECT_EQ("int", compiler::GetTypeName(typeid(int))); }

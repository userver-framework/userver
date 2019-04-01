#include <gtest/gtest.h>

#include <compiler/demangle.hpp>
#include <utest/utest.hpp>

TEST(Demangle, Int) { EXPECT_EQ("int", compiler::GetTypeName(typeid(int))); }

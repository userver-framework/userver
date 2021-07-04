#include <gtest/gtest.h>

#include <userver/compiler/demangle.hpp>

TEST(Demangle, Int) { EXPECT_EQ("int", compiler::GetTypeName(typeid(int))); }

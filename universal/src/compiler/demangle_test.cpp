#include <gtest/gtest.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Demangle, Int) { EXPECT_EQ("int", compiler::GetTypeName(typeid(int))); }

USERVER_NAMESPACE_END

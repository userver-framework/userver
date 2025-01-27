#include <gtest/gtest.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Demangle, DynamicInt) { EXPECT_EQ("int", compiler::GetTypeName(typeid(int))); }

TEST(Demangle, Int) { EXPECT_EQ("int", compiler::GetTypeName<int>()); }

TEST(Demangle, String) { EXPECT_EQ("std::string", compiler::GetTypeName<std::string>()); }

TEST(Demangle, IntRef) { EXPECT_EQ("int", compiler::GetTypeName<const int&>()); }

TEST(Demangle, StringRef) { EXPECT_EQ("std::string", compiler::GetTypeName<const std::string&>()); }

USERVER_NAMESPACE_END

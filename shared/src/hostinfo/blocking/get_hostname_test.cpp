#include <gtest/gtest.h>
#include <userver/hostinfo/blocking/get_hostname.hpp>

USERVER_NAMESPACE_BEGIN

TEST(GetRealHostName, Basic) {
  ASSERT_NO_THROW(hostinfo::blocking::GetRealHostName());
  EXPECT_FALSE(hostinfo::blocking::GetRealHostName().empty());
}

USERVER_NAMESPACE_END

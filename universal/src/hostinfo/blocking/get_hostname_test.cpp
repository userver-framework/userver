#include <gtest/gtest.h>
#include <userver/hostinfo/blocking/get_hostname.hpp>

USERVER_NAMESPACE_BEGIN

TEST(GetRealHostName, Basic) {
  const auto host_name = hostinfo::blocking::GetRealHostName();
  ASSERT_NO_THROW(hostinfo::blocking::GetRealHostName());
  EXPECT_FALSE(hostinfo::blocking::GetRealHostName().empty());
}

USERVER_NAMESPACE_END

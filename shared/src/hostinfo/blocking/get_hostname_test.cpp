#include <gtest/gtest.h>
#include <hostinfo/blocking/get_hostname.hpp>

TEST(GetRealHostName, Basic) {
  ASSERT_NO_THROW(hostinfo::blocking::GetRealHostName());
  EXPECT_FALSE(hostinfo::blocking::GetRealHostName().empty());
}

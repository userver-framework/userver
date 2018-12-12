#include <gtest/gtest.h>
#include <blocking/system/get_hostname.hpp>

TEST(GetRealHostName, Basic) {
  ASSERT_NO_THROW(blocking::system::GetRealHostName());
  EXPECT_FALSE(blocking::system::GetRealHostName().empty());
}

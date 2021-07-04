#include <userver/utils/uuid4.hpp>

#include <gtest/gtest.h>

TEST(UUID, String) {
  EXPECT_NE(utils::generators::GenerateUuid(), "");

  constexpr unsigned kUuidMinLength = 32;
  EXPECT_GE(utils::generators::GenerateUuid().size(), kUuidMinLength);

  EXPECT_NE(utils::generators::GenerateUuid(),
            utils::generators::GenerateUuid());
}

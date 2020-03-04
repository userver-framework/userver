#include <utils/uuid4.hpp>

#include <gtest/gtest.h>

TEST(UUID, String) {
  EXPECT_NE(utils::generators::GenerateUuid(), "");

  EXPECT_NE(utils::generators::GenerateUuid(),
            utils::generators::GenerateUuid());
}

#include <utils/boost_uuid4.hpp>

#include <gtest/gtest.h>

TEST(UUID, Boost) {
  EXPECT_NE(utils::generators::GenerateBoostUuid(), boost::uuids::uuid{});

  EXPECT_NE(utils::generators::GenerateBoostUuid(),
            utils::generators::GenerateBoostUuid());
}

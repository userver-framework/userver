#include <userver/utils/uuid7.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(UUIDv7, String) {
  EXPECT_NE(utils::generators::GenerateUuidV7(), "");

  constexpr unsigned kUuidMinLength = 32;
  EXPECT_GE(utils::generators::GenerateUuidV7().size(), kUuidMinLength);

  EXPECT_NE(utils::generators::GenerateUuidV7(),
            utils::generators::GenerateUuidV7());

  // Expect random data at the last 11 chars,
  // unlike in 0191c169-962a-8000-8000-000000000003
  constexpr std::string_view kBadData = "00000000000";
  for (int i = 0; i < 10; i++) {
    auto uuid = utils::generators::GenerateUuidV7();
    uuid = uuid.substr(uuid.size() - 12, 11);
    EXPECT_EQ(uuid.size(), kBadData.size());
    EXPECT_NE(uuid, kBadData);
    for (auto x : uuid) {
      EXPECT_TRUE(std::isdigit(x) || (x >= 'a' && x <= 'f')) << uuid;
    }
  }
}

USERVER_NAMESPACE_END

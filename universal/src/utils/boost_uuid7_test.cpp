#include <userver/utils/boost_uuid7.hpp>

#include <gtest/gtest.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

USERVER_NAMESPACE_BEGIN

TEST(UUIDv7, Basic) {
  EXPECT_NE(utils::generators::GenerateBoostUuidV7(), boost::uuids::uuid{});

  EXPECT_NE(utils::generators::GenerateBoostUuidV7(),
            utils::generators::GenerateBoostUuidV7());
}

TEST(UUIDv7, VersionAndVariant) {
  const auto uuid = utils::generators::GenerateBoostUuidV7();

  EXPECT_EQ(uuid.variant(), boost::uuids::uuid::variant_rfc_4122);

  EXPECT_EQ(uuid.data[6] & 0xF0, 0x70);
}

TEST(UUIDv7, Ordered) {
  static constexpr auto kUuidsToGenerate = 10'000'000;

  std::vector<boost::uuids::uuid> uuids;
  uuids.reserve(kUuidsToGenerate);

  for (auto i = 0; i < kUuidsToGenerate; ++i) {
    uuids.push_back(utils::generators::GenerateBoostUuidV7());
  }

  // sequentially generated uuids v7 should be ordered and unique
  for (size_t i = 0; i < uuids.size() - 1; ++i) {
    EXPECT_LT(uuids[i], uuids[i + 1])
        << "uuids[" << i << "]=" << uuids[i] << " should be less than uuids["
        << i + 1 << "]=" << uuids[i + 1];
  }
}

USERVER_NAMESPACE_END

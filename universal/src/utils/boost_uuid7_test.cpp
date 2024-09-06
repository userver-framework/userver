#include <userver/utils/boost_uuid7.hpp>

#include <gtest/gtest.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::uint64_t ExtractMonoticCounterFromUuid(boost::uuids::uuid uuid) {
  return (static_cast<std::uint32_t>(uuid.data[6] & 0x0F) << 18) +
         (static_cast<std::uint32_t>(uuid.data[7]) << 10) +
         (static_cast<std::uint32_t>(uuid.data[8] & 0x3F) << 4) +
         (static_cast<std::uint32_t>(uuid.data[9]) >> 4);
}

}  // namespace

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

TEST(UUIDv7, MonoticTimestamp) {
  static constexpr auto kUuidsToGenerate = 1'000;

  std::vector<boost::uuids::uuid> uuids;
  uuids.reserve(kUuidsToGenerate);

  for (auto i = 0; i < kUuidsToGenerate; ++i) {
    uuids.push_back(utils::generators::GenerateBoostUuidV7());
  }

  // sequentially generated uuids v7 should be ordered and unique
  for (size_t i = 0; i < uuids.size() - 1; ++i) {
    EXPECT_LE(utils::ExtractTimestampFromUuidV7(uuids[i]),
              utils::ExtractTimestampFromUuidV7(uuids[i + 1]))
        << "uuids[" << i << "]=" << uuids[i]
        << " timestamp should be less than uuids[" << i + 1
        << "]=" << uuids[i + 1] << " timestmap";
  }
}

TEST(UUIDv7, MonoticCounter) {
  auto uuid1 = boost::uuids::uuid{};
  auto uuid2 = boost::uuids::uuid{};

  do {
    uuid1 = utils::generators::GenerateBoostUuidV7();
    uuid2 = utils::generators::GenerateBoostUuidV7();
  } while (utils::ExtractTimestampFromUuidV7(uuid1) !=
           utils::ExtractTimestampFromUuidV7(uuid2));

  const auto uuid1_counter = ExtractMonoticCounterFromUuid(uuid1);
  const auto uuid2_counter = ExtractMonoticCounterFromUuid(uuid2);

  EXPECT_EQ(uuid1_counter + 1, uuid2_counter);
}

TEST(UUIDv7, RandomBlockExists) {
  const auto uuid1 = utils::generators::GenerateBoostUuidV7();
  const auto uuid2 = utils::generators::GenerateBoostUuidV7();

  // Actually 4 bits from 9-th byte are random too, but it's safe to assume
  // that comparing all other bytes are enough to ensure randomness of block.
  const auto uuid1_block =
      utils::span<const std::uint8_t>(uuid1.begin(), uuid1.end()).subspan(10);
  const auto uuid2_block =
      utils::span<const std::uint8_t>(uuid2.begin(), uuid2.end()).subspan(10);

  ASSERT_EQ(uuid1_block.size(), uuid2_block.size());

  std::size_t equal_symbols = 0;
  for (std::size_t idx = 0; idx < uuid1_block.size(); ++idx) {
    if (uuid1_block[idx] == uuid2_block[idx]) {
      ++equal_symbols;
    }
  }

  EXPECT_NE(equal_symbols, uuid1_block.size());
}

USERVER_NAMESPACE_END

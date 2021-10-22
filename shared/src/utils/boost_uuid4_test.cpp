#include <userver/utils/boost_uuid4.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include <gtest/gtest.h>

#include <string>

USERVER_NAMESPACE_BEGIN

TEST(UUID, Boost) {
  EXPECT_NE(utils::generators::GenerateBoostUuid(), boost::uuids::uuid{});

  EXPECT_NE(utils::generators::GenerateBoostUuid(),
            utils::generators::GenerateBoostUuid());
}

TEST(UUID, Format) {
  std::string str("0ad56dfc-bbbf-44af-87e3-37eb98b6452f");
  boost::uuids::string_generator string_gen;
  EXPECT_EQ(str, fmt::format("{}", string_gen(str)));

  auto id = utils::generators::GenerateBoostUuid();
  EXPECT_EQ(utils::generators::impl::ToString(id), fmt::format("{}", id));
}

USERVER_NAMESPACE_END

#include <userver/utils/boost_uuid4.hpp>

#include <string>

#include <gtest/gtest.h>
#include <boost/uuid/uuid_generators.hpp>

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
  EXPECT_EQ(utils::ToString(id), fmt::format("{}", id));
}

TEST(UUID, ParseOk) {
  std::vector<std::string> spelling_variants(
      {"0ad56dfc-bbbf-44af-87e3-37eb98b6452f",
       "0ad56dfcbbbf44af87e337eb98b6452f",
       "{0ad56dfc-bbbf-44af-87e3-37eb98b6452f}",
       "{0ad56dfcbbbf44af87e337eb98b6452f}"});

  std::string expected_format("0ad56dfc-bbbf-44af-87e3-37eb98b6452f");
  for (const auto& str : spelling_variants) {
    auto id = utils::BoostUuidFromString(str);

    EXPECT_EQ(expected_format, fmt::format("{}", id));
  }
}

TEST(UUID, ParseFail) {
  std::vector<std::string> invalid_uuids({
      "0ad56dfc-bbbf-44af-87e337eb98b6452f",
      "0ad56dfcbbbf44af87e337eb98-b6452f",
      "{xad56dfc-bbbf-44af-87e3-37eb98b6452f}",
      "0ad56dfcbbbf44af87e337eb98b6452f}",
      "{0ad56dfcbbbf44af87e337eb98b6452f",
      "{0ad56dfc-bbbf-44af-87e3-37eb98b6452f}a",
      "0ad56dfc-bbbf-44af-",
  });
  try {
    std::string str{"xad56dfc-bbbf-44af-87e3-37eb98b6452f"};
    utils::BoostUuidFromString(str);
    FAIL() << "Must throw on invalid UUID " << str;
  } catch (const std::runtime_error& e) {
    EXPECT_EQ(std::string{"Invalid uuid string"}, e.what());
  }
}

USERVER_NAMESPACE_END

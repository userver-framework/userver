#include <gtest/gtest.h>

#include <userver/utils/regex.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Regex, Ctors) {
  utils::regex r1;
  utils::regex r2("regex*test");
  utils::regex r3(std::move(r2));
  utils::regex r4(r3);
  utils::regex r5;
  r5 = std::move(r4);
  utils::regex r6;
  r6 = r5;
}

TEST(Regex, Match) {
  utils::regex r("^[a-z][0-9]+");
  EXPECT_FALSE(utils::regex_match({}, r));
  EXPECT_FALSE(utils::regex_match("a", r));
  EXPECT_FALSE(utils::regex_match("123", r));
  EXPECT_TRUE(utils::regex_match("a123", r));
  EXPECT_TRUE(utils::regex_match("a1234", r));
  EXPECT_FALSE(utils::regex_match("a123a", r));
}

TEST(Regex, Search) {
  utils::regex r("^[a-z][0-9]+");
  EXPECT_FALSE(utils::regex_search({}, r));
  EXPECT_FALSE(utils::regex_search("a", r));
  EXPECT_FALSE(utils::regex_search("123", r));
  EXPECT_TRUE(utils::regex_search("a123", r));
  EXPECT_TRUE(utils::regex_search("a1234", r));
  EXPECT_TRUE(utils::regex_search("a123a", r));
}

USERVER_NAMESPACE_END

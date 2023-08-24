#include <userver/utils/str_icase_containers.hpp>

#include <string>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(WithSafeHash, Map) {
  EXPECT_EQ(utils::WithSafeHash(
                std::unordered_map<std::string, std::string>{{"foo", "bar"}}),
            (std::unordered_map<std::string, std::string, utils::StrCaseHash>{
                {"foo", "bar"}}));

  EXPECT_EQ(
      utils::WithSafeHash(
          std::unordered_multimap<std::string, std::string>{{"foo", "bar"}}),
      (std::unordered_multimap<std::string, std::string, utils::StrCaseHash>{
          {"foo", "bar"}}));
}

TEST(WithSafeHash, Set) {
  EXPECT_EQ(utils::WithSafeHash(
                std::unordered_map<std::string, std::string>{{"foo", "bar"}}),
            (std::unordered_map<std::string, std::string, utils::StrCaseHash>{
                {"foo", "bar"}}));

  EXPECT_EQ(
      utils::WithSafeHash(
          std::unordered_multimap<std::string, std::string>{{"foo", "bar"}}),
      (std::unordered_multimap<std::string, std::string, utils::StrCaseHash>{
          {"foo", "bar"}}));
}

USERVER_NAMESPACE_END

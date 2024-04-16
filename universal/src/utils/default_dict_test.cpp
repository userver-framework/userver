#include <userver/utils/default_dict.hpp>

#include <gtest/gtest.h>
#include <boost/range/adaptor/map.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/yaml/value.hpp>

USERVER_NAMESPACE_BEGIN

TEST(DefaultDict, UseAsRange) {
  using DefaultDict = utils::DefaultDict<int>;

  DefaultDict m_dict{
      "name",
      {{"one", 1}, {"two", 2}, {"three", 3}},
  };
  const DefaultDict& c_dict = m_dict;

  EXPECT_TRUE(
      (std::is_same_v<decltype(m_dict.begin()), DefaultDict::iterator>));
  EXPECT_TRUE(
      (std::is_same_v<decltype(c_dict.begin()), DefaultDict::const_iterator>));

  // DefaultDict has no mutable iterator: iterator == const_iterator.
  EXPECT_TRUE(
      (std::is_same_v<decltype(m_dict.begin()), decltype(c_dict.begin())>));

  for (const auto& each : (m_dict | boost::adaptors::map_keys)) {
    EXPECT_TRUE(m_dict.HasValue(each));
  }

  for (const auto& each : (c_dict | boost::adaptors::map_keys)) {
    EXPECT_TRUE(c_dict.HasValue(each));
  }
}

TEST(DefaultDict, ParseFromJson) {
  using DefaultDict = utils::DefaultDict<std::string>;
  using formats::literals::operator""_json;

  DefaultDict expected_dict{
      "",
      {{"a", "one"}, {"b", "two"}, {"c", "three"}},
  };

  EXPECT_EQ(expected_dict, R"json(
  {"a": "one", "b": "two", "c": "three"})json"_json.As<DefaultDict>());
}

TEST(DefaultDict, ParseFromYaml) {
  using DefaultDict = utils::DefaultDict<std::string>;
  using formats::literals::operator""_yaml;

  DefaultDict expected_dict{
      "",
      {{"a", "one"}, {"b", "two"}, {"c", "three"}},
  };

  EXPECT_EQ(expected_dict, R"yaml(
    a: one
    b: two
    c: three
  )yaml"_yaml.As<DefaultDict>());
}

USERVER_NAMESPACE_END

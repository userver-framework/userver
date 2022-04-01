#include <userver/dynamic_config/value.hpp>

#include <gtest/gtest.h>
#include <boost/range/adaptor/map.hpp>

USERVER_NAMESPACE_BEGIN

TEST(DocsMap, AreContentsEqualTrue) {
  dynamic_config::DocsMap docs_map1;
  docs_map1.Parse(R"({"a": "a", "b": "b"})", false);
  (void)docs_map1.Get("a");

  dynamic_config::DocsMap docs_map2;
  docs_map2.Parse(R"({"b": "b", "a": "a"})", false);
  (void)docs_map2.Get("b");

  EXPECT_NE(docs_map1.GetRequestedNames(), docs_map2.GetRequestedNames());
  EXPECT_TRUE(docs_map1.AreContentsEqual(docs_map2));
}

TEST(DocsMap, AreContentsEqualFalse) {
  dynamic_config::DocsMap docs_map1;
  docs_map1.Parse(R"({"a": "a", "b": "b"})", false);

  dynamic_config::DocsMap docs_map2;
  docs_map2.Parse(R"({"a": "b", "b": "a"})", false);

  EXPECT_EQ(docs_map1.GetRequestedNames(), docs_map2.GetRequestedNames());
  EXPECT_FALSE(docs_map1.AreContentsEqual(docs_map2));
}

TEST(ValueDict, UseAsRange) {
  using ValueDict = dynamic_config::ValueDict<int>;

  ValueDict m_dict{
      "name",
      {{"one", 1}, {"two", 2}, {"three", 3}},
  };
  const ValueDict& c_dict = m_dict;

  EXPECT_TRUE((std::is_same_v<decltype(m_dict.begin()), ValueDict::iterator>));
  EXPECT_TRUE(
      (std::is_same_v<decltype(c_dict.begin()), ValueDict::const_iterator>));

  // ValueDict has no mutable iterator: iterator == const_iterator.
  EXPECT_TRUE(
      (std::is_same_v<decltype(m_dict.begin()), decltype(c_dict.begin())>));

  for (const auto& each : (m_dict | boost::adaptors::map_keys)) {
    EXPECT_TRUE(m_dict.HasValue(each));
  }

  for (const auto& each : (c_dict | boost::adaptors::map_keys)) {
    EXPECT_TRUE(c_dict.HasValue(each));
  }
}

USERVER_NAMESPACE_END

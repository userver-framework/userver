#include <userver/dynamic_config/value.hpp>

#include <gtest/gtest.h>
#include <boost/range/adaptor/map.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/value.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

TEST(DocsMap, HasConfig) {
  dynamic_config::DocsMap docs_map;
  docs_map.Parse(R"({"a": "a", "b": "b"})", false);

  EXPECT_TRUE(docs_map.Has("a"));
  EXPECT_FALSE(docs_map.Has("c"));
  EXPECT_EQ(docs_map.Get("a"), formats::json::ValueBuilder("a").ExtractValue());
}

TEST(DocsMap, AreContentsEqualTrue) {
  dynamic_config::DocsMap docs_map1;
  docs_map1.SetConfigsExpectedToBeUsed({"a"}, utils::InternalTag{});
  docs_map1.Parse(R"({"a": "a", "b": "b"})", false);

  dynamic_config::DocsMap docs_map2;
  docs_map2.SetConfigsExpectedToBeUsed({"b"}, utils::InternalTag{});
  docs_map2.Parse(R"({"b": "b", "a": "a"})", false);

  EXPECT_NE(docs_map1.GetConfigsExpectedToBeUsed(utils::InternalTag{}),
            docs_map2.GetConfigsExpectedToBeUsed(utils::InternalTag{}));
  EXPECT_TRUE(docs_map1.AreContentsEqual(docs_map2));
}

TEST(DocsMap, AreContentsEqualFalse) {
  dynamic_config::DocsMap docs_map1;
  docs_map1.Parse(R"({"a": "a", "b": "b"})", false);

  dynamic_config::DocsMap docs_map2;
  docs_map2.Parse(R"({"a": "b", "b": "a"})", false);

  EXPECT_EQ(docs_map1.GetConfigsExpectedToBeUsed(utils::InternalTag{}),
            docs_map2.GetConfigsExpectedToBeUsed(utils::InternalTag{}));
  EXPECT_FALSE(docs_map1.AreContentsEqual(docs_map2));
}

TEST(DocsMap, ConfigExpectedToBeUsedRemovedAfterGet) {
  dynamic_config::DocsMap docs_map;
  utils::impl::TransparentSet<std::string> to_be_used = {"a", "b"};
  docs_map.SetConfigsExpectedToBeUsed(
      utils::impl::TransparentSet<std::string>(to_be_used),
      utils::InternalTag{});
  docs_map.Parse(R"({"a": "a", "b": "b"})", false);

  EXPECT_EQ(docs_map.GetConfigsExpectedToBeUsed(utils::InternalTag{}),
            to_be_used);

  (void)docs_map.Get("a");

  EXPECT_EQ(docs_map.GetConfigsExpectedToBeUsed(utils::InternalTag{}),
            utils::impl::TransparentSet<std::string>({"b"}));

  dynamic_config::DocsMap docs_map_copy(docs_map);
  (void)docs_map_copy.Get("b");

  EXPECT_TRUE(
      docs_map_copy.GetConfigsExpectedToBeUsed(utils::InternalTag{}).empty());
}

TEST(DocsMap, Merge) {
  dynamic_config::DocsMap docs_map1;
  docs_map1.Parse(R"({"a": "a", "b": "b"})", false);
  dynamic_config::DocsMap docs_map2;
  docs_map2.Parse(R"({"a": "x", "c": "c"})", false);
  dynamic_config::DocsMap docs_map3;
  docs_map3.Parse(R"({"b": "x", "c": "x", "d": "d"})", false);

  docs_map1.MergeOrAssign(std::move(docs_map2));

  EXPECT_EQ(docs_map1.Size(), 3);
  EXPECT_EQ(docs_map1.Get("a").As<std::string>(), "x");
  EXPECT_EQ(docs_map1.Get("b").As<std::string>(), "b");
  EXPECT_EQ(docs_map1.Get("c").As<std::string>(), "c");

  docs_map1.MergeMissing(docs_map3);

  EXPECT_EQ(docs_map1.Size(), 4);
  EXPECT_EQ(docs_map1.Get("a").As<std::string>(), "x");
  EXPECT_EQ(docs_map1.Get("b").As<std::string>(), "b");
  EXPECT_EQ(docs_map1.Get("c").As<std::string>(), "c");
  EXPECT_EQ(docs_map1.Get("d").As<std::string>(), "d");
}

USERVER_NAMESPACE_END

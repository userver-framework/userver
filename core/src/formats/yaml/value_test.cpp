#include <formats/yaml/serialize.hpp>
#include <formats/yaml/serialize_container.hpp>
#include <formats/yaml/value.hpp>
#include <utest/utest.hpp>

TEST(YamlParse, ContainersCtr) {
  auto value = formats::yaml::FromString("null");

  EXPECT_TRUE(value.As<std::unordered_set<int>>().empty());
  EXPECT_TRUE(value.As<std::vector<int>>().empty());
  EXPECT_TRUE(value.As<std::set<int>>().empty());
  EXPECT_TRUE((value.As<std::map<std::string, int>>().empty()));
  EXPECT_TRUE((value.As<std::unordered_map<std::string, int>>().empty()));
}

TEST(YamlParse, VectorInt) {
  auto value = formats::yaml::FromString("[1,2,3]");
  std::vector<int> v = value.As<std::vector<int>>();
  EXPECT_EQ((std::vector<int>{1, 2, 3}), v);
}

TEST(YamlParse, VectorIntNull) {
  auto value = formats::yaml::FromString("null");
  std::vector<int> v = value.As<std::vector<int>>();
  EXPECT_EQ(std::vector<int>{}, v);
}

TEST(YamlParse, VectorIntErrorObj) {
  auto value = formats::yaml::FromString("{\"1\": 1}");
  EXPECT_THROW(value.As<std::vector<int>>(), std::exception);
}

TEST(YamlParse, VectorVectorInt) {
  auto value = formats::yaml::FromString("[[1,2,3],[4,5],[6],[]]");
  std::vector<std::vector<int>> v = value.As<std::vector<std::vector<int>>>();
  EXPECT_EQ((std::vector<std::vector<int>>{{1, 2, 3}, {4, 5}, {6}, {}}), v);
}

TEST(YamlParse, VectorVectorIntNull) {
  auto value = formats::yaml::FromString("null");
  std::vector<std::vector<int>> v = value.As<std::vector<std::vector<int>>>();
  EXPECT_EQ(std::vector<std::vector<int>>{}, v);
}

TEST(YamlParse, OptionalIntNone) {
  auto value = formats::yaml::FromString("{}")["nonexisting"];
  boost::optional<int> v = value.As<boost::optional<int>>();
  EXPECT_EQ(boost::none, v);
}

TEST(YamlParse, OptionalInt) {
  auto value = formats::yaml::FromString("[1]")[0];
  boost::optional<int> v = value.As<boost::optional<int>>();
  EXPECT_EQ(1, v);
}

TEST(YamlParse, OptionalVectorInt) {
  auto value = formats::yaml::FromString("{}")["nonexisting"];
  boost::optional<std::vector<int>> v =
      value.As<boost::optional<std::vector<int>>>();
  EXPECT_EQ(boost::none, v);
}

TEST(YamlParse, Int) {
  auto value = formats::yaml::FromString("[32768]")[0];
  EXPECT_THROW(value.As<int16_t>(), std::exception);

  value = formats::yaml::FromString("[32767]")[0];
  EXPECT_EQ(32767, value.As<int16_t>());
}

TEST(YamlParse, UInt) {
  auto value = formats::yaml::FromString("[-1]")[0];
  EXPECT_THROW(value.As<unsigned int>(), std::exception);

  value = formats::yaml::FromString("[0]")[0];
  EXPECT_EQ(0, value.As<unsigned int>());
}

TEST(YamlParse, IntOverflow) {
  auto value = formats::yaml::FromString("[65536]")[0];
  EXPECT_THROW(value.As<uint16_t>(), std::exception);

  value = formats::yaml::FromString("[65535]")[0];
  EXPECT_EQ(65535u, value.As<uint16_t>());
}

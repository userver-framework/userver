#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/serialize_container.hpp>
#include <formats/json/value.hpp>
#include <utest/utest.hpp>

TEST(JsonParse, ContainersCtr) {
  auto value = formats::json::FromString("null");

  EXPECT_TRUE(value.As<std::unordered_set<int>>().empty());
  EXPECT_TRUE(value.As<std::vector<int>>().empty());
  EXPECT_TRUE(value.As<std::set<int>>().empty());
  EXPECT_TRUE((value.As<std::map<std::string, int>>().empty()));
  EXPECT_TRUE((value.As<std::unordered_map<std::string, int>>().empty()));
}

TEST(JsonParse, VectorInt) {
  auto value = formats::json::FromString("[1,2,3]");
  std::vector<int> v = value.As<std::vector<int>>();
  EXPECT_EQ((std::vector<int>{1, 2, 3}), v);
}

TEST(JsonParse, VectorIntNull) {
  auto value = formats::json::FromString("null");
  std::vector<int> v = value.As<std::vector<int>>();
  EXPECT_EQ(std::vector<int>{}, v);
}

TEST(JsonParse, VectorIntErrorObj) {
  auto value = formats::json::FromString("{\"1\": 1}");
  EXPECT_THROW(value.As<std::vector<int>>(), std::exception);
}

TEST(JsonParse, VectorVectorInt) {
  auto value = formats::json::FromString("[[1,2,3],[4,5],[6],[]]");
  std::vector<std::vector<int>> v = value.As<std::vector<std::vector<int>>>();
  EXPECT_EQ((std::vector<std::vector<int>>{{1, 2, 3}, {4, 5}, {6}, {}}), v);
}

TEST(JsonParse, VectorVectorIntNull) {
  auto value = formats::json::FromString("null");
  std::vector<std::vector<int>> v = value.As<std::vector<std::vector<int>>>();
  EXPECT_EQ(std::vector<std::vector<int>>{}, v);
}

TEST(JsonParse, OptionalIntNone) {
  auto value = formats::json::FromString("{}")["nonexisting"];
  boost::optional<int> v = value.As<boost::optional<int>>();
  EXPECT_EQ(boost::none, v);
}

TEST(JsonParse, OptionalInt) {
  auto value = formats::json::FromString("[1]")[0];
  boost::optional<int> v = value.As<boost::optional<int>>();
  EXPECT_EQ(1, v);
}

TEST(JsonParse, OptionalVectorInt) {
  auto value = formats::json::FromString("{}")["nonexisting"];
  boost::optional<std::vector<int>> v =
      value.As<boost::optional<std::vector<int>>>();
  EXPECT_EQ(boost::none, v);
}

TEST(JsonParse, Int) {
  auto value = formats::json::FromString("[32768]")[0];
  EXPECT_THROW(value.As<int16_t>(), std::exception);

  value = formats::json::FromString("[32767]")[0];
  EXPECT_EQ(32767, value.As<int16_t>());
}

TEST(JsonParse, UInt) {
  auto value = formats::json::FromString("[-1]")[0];
  EXPECT_THROW(value.As<unsigned int>(), std::exception);

  value = formats::json::FromString("[0]")[0];
  EXPECT_EQ(0, value.As<unsigned int>());
}

TEST(JsonParse, IntOverflow) {
  auto value = formats::json::FromString("[65536]")[0];
  EXPECT_THROW(value.As<uint16_t>(), std::exception);

  value = formats::json::FromString("[65535]")[0];
  EXPECT_EQ(65535u, value.As<uint16_t>());
}

TEST(JsonParse, Default) {
  auto value = formats::json::FromString(R"({"existing": "yes"})");
  EXPECT_EQ("yes", value["existing"].As<std::string>());
  EXPECT_EQ("yes", value["existing"].As<std::string>(std::string{"no"}));
  EXPECT_EQ("yes", value["existing"].As<std::string>("no"));
  EXPECT_THROW(value["existing"].As<int>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(value["existing"].As<int>(0),
               formats::json::TypeMismatchException);

  EXPECT_THROW(value["nonexistent"].As<std::string>(),
               formats::json::MemberMissingException);
  EXPECT_EQ("no", value["nonexistent"].As<std::string>(std::string("no")));
  EXPECT_EQ("no", value["nonexistent"].As<std::string>("no"));
  EXPECT_EQ("no", value["nonexistent"].As<std::string>("nope", 2));
  EXPECT_THROW(value["nonexistent"].As<int>(),
               formats::json::MemberMissingException);
  EXPECT_EQ(0, value["nonexistent"].As<int>(0));
}

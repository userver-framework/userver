#include <userver/utest/assert_macros.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/parse/common_containers.hpp>

#include <userver/chaotic/oneof_with_discriminator.hpp>
#include <userver/chaotic/primitive.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

static const auto kJsonShort =
    formats::json::MakeObject("type", "ObjectStringWithDiscriminator");

static const auto kJson1 =
    formats::json::MakeObject("type", "ObjectStringWithDiscriminator", "value",
                              "What is the meaning of life?");

static const auto kJson2 = formats::json::MakeObject(
    "type", "ObjectIntWithDiscriminator", "value", 42);

static const auto kJsonBad =
    formats::json::MakeObject("type", "SomeObject", "value", 42);

struct ObjectIntWithDiscriminator {
  std::optional<std::string> type;
  std::optional<int> value;
};

ObjectIntWithDiscriminator Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<ObjectIntWithDiscriminator>) {
  json.CheckNotMissing();
  json.CheckObjectOrNull();

  ObjectIntWithDiscriminator res;

  res.type = json["type"]
                 .As<std::optional<
                     USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
  res.value =
      json["value"]
          .As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

  return res;
}

struct ObjectStringWithDiscriminator {
  std::optional<std::string> type;
  std::optional<std::string> value;
};

ObjectStringWithDiscriminator Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<ObjectStringWithDiscriminator>) {
  json.CheckNotMissing();
  json.CheckObjectOrNull();

  ObjectStringWithDiscriminator res;

  res.type = json["type"]
                 .As<std::optional<
                     USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
  res.value = json["value"]
                  .As<std::optional<
                      USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();

  return res;
}

}  // namespace

TEST(OneOfWithDiscriminator, Simple) {
  static constexpr chaotic::OneOfSettings kSettings{
      "type", utils::TrivialSet([](auto selector) {
        return selector()
            .Case("ObjectIntWithDiscriminator")
            .Case("ObjectStringWithDiscriminator");
      })};

  using OneOfWithDiscriminator =
      chaotic::OneOfWithDiscriminator<&kSettings, ObjectIntWithDiscriminator,
                                      ObjectStringWithDiscriminator>;

  UEXPECT_NO_THROW(kJson1.As<OneOfWithDiscriminator>());

  auto oneof1 = kJson1.As<OneOfWithDiscriminator>();
  EXPECT_EQ(oneof1.index(), std::size_t{1});
  EXPECT_EQ(std::get<1>(oneof1).value, "What is the meaning of life?");

  UEXPECT_NO_THROW(kJson2.As<OneOfWithDiscriminator>());

  auto oneof2 = kJson2.As<OneOfWithDiscriminator>();
  EXPECT_EQ(oneof2.index(), std::size_t{0});
  EXPECT_EQ(std::get<0>(oneof2).value, 42);
}

TEST(OneOfWithDiscriminator, RepeatedTypes) {
  static constexpr chaotic::OneOfSettings kSettings{
      "type", utils::TrivialSet([](auto selector) {
        return selector()
            .Case("ObjectIntWithDiscriminator")
            .Case("ObjectIntWithDiscriminator")
            .Case("ObjectStringWithDiscriminator")
            .Case("ObjectIntWithDiscriminator")
            .Case("ObjectStringWithDiscriminator");
      })};

  using OneOfWithDiscriminator = chaotic::OneOfWithDiscriminator<
      &kSettings, ObjectIntWithDiscriminator, ObjectIntWithDiscriminator,
      ObjectStringWithDiscriminator, ObjectIntWithDiscriminator,
      ObjectStringWithDiscriminator>;

  UEXPECT_NO_THROW(kJson1.As<OneOfWithDiscriminator>());
  auto oneof1 = kJson1.As<OneOfWithDiscriminator>();
  EXPECT_EQ(oneof1.index(), std::size_t{2});
  EXPECT_EQ(std::get<2>(oneof1).value, "What is the meaning of life?");

  UEXPECT_NO_THROW(kJson2.As<OneOfWithDiscriminator>());
  auto oneof2 = kJson2.As<OneOfWithDiscriminator>();
  EXPECT_EQ(oneof2.index(), std::size_t{0});
  EXPECT_EQ(std::get<0>(oneof2).value, 42);
}

TEST(OneOfWithDiscriminator, ParseError) {
  static constexpr chaotic::OneOfSettings kSettings{
      "type", utils::TrivialSet([](auto selector) {
        return selector()
            .Case("ObjectIntWithDiscriminator")
            .Case("ObjectStringWithDiscriminator");
      })};

  using OneOfWithDiscriminator =
      chaotic::OneOfWithDiscriminator<&kSettings, ObjectIntWithDiscriminator,
                                      ObjectStringWithDiscriminator>;

  UEXPECT_THROW_MSG(
      kJsonBad.As<OneOfWithDiscriminator>(),
      formats::json::UnknownDiscriminatorException,
      "Error at path '/': Unknown discriminator field value 'SomeObject'");
}

USERVER_NAMESPACE_END

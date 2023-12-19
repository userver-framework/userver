#include <gtest/gtest.h>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/boost_uuid.hpp>

USERVER_NAMESPACE_BEGIN

TEST(FormatsJson, UUID) {
  using formats::json::FromString;

  auto val =
      FromString(R"~({"uuid" : "4ece2fb4-94bf-11e9-b7ff-ac1f6b8569b3"})~");
  EXPECT_NO_THROW(val["uuid"].As<boost::uuids::uuid>());

  val = FromString(R"~({"uuid" : "4ece2fb494bf11e9b7ffac1f6b8569b3"})~");
  EXPECT_NO_THROW(val["uuid"].As<boost::uuids::uuid>())
      << "No dashes are allowed";

  val = FromString(R"~({"uuid" : "{4ece2fb4-94bf-11e9-b7ff-ac1f6b8569b3}"})~");
  EXPECT_NO_THROW(val["uuid"].As<boost::uuids::uuid>())
      << "Curly braces are allowed";

  val = FromString(R"~({"uuid" : "{4ece2fb494bf11e9b7ffac1f6b8569b3}"})~");
  EXPECT_NO_THROW(val["uuid"].As<boost::uuids::uuid>())
      << "Curly braces are allowed";

  val = FromString(R"~({"uuid" : "4ece2fb494bf-11e9-b7ff-ac1f6b8569b3"})~");
  EXPECT_THROW(val["uuid"].As<boost::uuids::uuid>(),
               formats::json::ParseException)
      << "Dashes must be consistent";

  val = FromString(R"~({"uuid" : "4ece2fb494bf-11e9b7ff-ac1f6b8569b3"})~");
  EXPECT_THROW(val["uuid"].As<boost::uuids::uuid>(),
               formats::json::ParseException)
      << "Dashes must be consistent";

  val = FromString(R"~({"uuid" : "4ece2fb4-94bf11e9-b7ff-ac1f6b8569b3"})~");
  EXPECT_THROW(val["uuid"].As<boost::uuids::uuid>(),
               formats::json::ParseException)
      << "Dashes must be consistent";
}

USERVER_NAMESPACE_END

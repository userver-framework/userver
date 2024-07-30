#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/inline.hpp>

#include <schemas/int_minmax.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MinMax, ExclusiveInt) {
  auto json = formats::json::MakeObject("foo", 1);
  UEXPECT_THROW_MSG(
      json.As<ns::IntegerObject>(), chaotic::Error,
      "Error at path 'foo': Invalid value, exclusive minimum=1, given=1");

  json = formats::json::MakeObject("foo", 2);
  EXPECT_EQ(json.As<ns::IntegerObject>().foo, 2);

  json = formats::json::MakeObject("foo", 20);
  UEXPECT_THROW_MSG(
      json.As<ns::IntegerObject>(), chaotic::Error,
      "Error at path 'foo': Invalid value, exclusive maximum=20, given=20");

  json = formats::json::MakeObject("foo", 19);
  EXPECT_EQ(json.As<ns::IntegerObject>().foo, 19);
}

TEST(MinMax, String) {
  auto json = formats::json::MakeObject("bar", "");
  UEXPECT_THROW_MSG(
      json.As<ns::IntegerObject>(), chaotic::Error,
      "Error at path 'bar': Too short string, minimum length=2, given=0");

  json = formats::json::MakeObject("bar", "longlonglong");
  UEXPECT_THROW_MSG(
      json.As<ns::IntegerObject>(), chaotic::Error,
      "Error at path 'bar': Too long string, maximum length=5, given=12");
}

TEST(MinMax, Array) {
  auto json = formats::json::MakeObject("zoo", formats::json::MakeArray(1));
  UEXPECT_THROW_MSG(
      json.As<ns::IntegerObject>(), chaotic::Error,
      "Error at path 'zoo': Too short array, minimum length=2, given=1");

  json = formats::json::MakeObject(
      "zoo", formats::json::MakeArray(1, 2, 3, 4, 5, 6, 7, 8));
  UEXPECT_THROW_MSG(
      json.As<ns::IntegerObject>(), chaotic::Error,
      "Error at path 'zoo': Too long array, maximum length=5, given=8");
}

USERVER_NAMESPACE_END

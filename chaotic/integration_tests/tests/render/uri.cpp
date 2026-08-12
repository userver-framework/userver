#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/variant.hpp>

#include <userver/chaotic/exception.hpp>

#include <schemas/uri.hpp>
#include <schemas/uri_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, Uri) {
    auto uri = "http://example.com";
    auto json = formats::json::MakeObject("uri", uri);
    auto obj = json.As<ns::ObjectUri>();

    EXPECT_EQ(obj.uri, uri);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_EQ(TestWriteToStream(obj), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["uri"].As<std::string>();
    EXPECT_EQ(str, uri);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, UriSax) {
    auto uri = "http://example.com";
    auto json = formats::json::MakeObject("uri", uri);
    auto obj = CallSaxParser<ns::ObjectUri>(ToString(json));
    EXPECT_EQ(obj.uri, uri);
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Simple, UriInvalid) {
    auto json = formats::json::MakeObject("uri", "example.com");
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectUri>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'uri': doesn't match regex"
    );
}

USERVER_NAMESPACE_END

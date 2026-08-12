#include <gmock/gmock.h>

#include <userver/formats/json/inline.hpp>

#include <boost/uuid/string_generator.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/variant.hpp>

#include <userver/chaotic/exception.hpp>

#include <schemas/uuid.hpp>
#include <schemas/uuid_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, Uuid) {
    auto uuid = "01234567-89ab-cdef-0123-456789abcdef";
    auto json = formats::json::MakeObject("uuid", uuid);
    auto obj = json.As<ns::ObjectUuid>();

    const boost::uuids::string_generator gen;
    const boost::uuids::uuid expected = gen(uuid);
    EXPECT_EQ(obj.uuid, expected);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_EQ(TestWriteToStream(obj), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["uuid"].As<std::string>();
    EXPECT_EQ(str, uuid);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, UuidSax) {
    auto uuid = "01234567-89ab-cdef-0123-456789abcdef";
    auto json = formats::json::MakeObject("uuid", uuid);
    auto obj = CallSaxParser<ns::ObjectUuid>(ToString(json));

    const boost::uuids::string_generator gen;
    EXPECT_EQ(obj.uuid, gen(uuid));
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Simple, UuidInvalid) {
    auto json = formats::json::MakeObject("uuid", "not-a-valid-uuid");
    EXPECT_THAT(
        [&] { json.As<ns::ObjectUuid>(); },
        testing::ThrowsMessage<chaotic::Error<formats::json::Value>>(testing::AnyOf(
            testing::HasSubstr("Error at path 'uuid': Invalid UUID string at position 0: hex digit expected"),
            testing::HasSubstr("Error at path 'uuid': invalid uuid string")
        ))
    );
}

USERVER_NAMESPACE_END

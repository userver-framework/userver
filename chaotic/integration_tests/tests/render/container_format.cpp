#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <boost/version.hpp>

#include <userver/chaotic/exception.hpp>

#include <schemas/container_format.hpp>
#include <schemas/container_format_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(ContainerFormat, UuidValid) {
    auto json = formats::json::MakeObject("my_field", formats::json::MakeArray("01234567-89ab-cdef-0123-456789abcdef"));
    auto obj = json.As<ns::ContainerWithFormatItem>();
    EXPECT_EQ(obj.my_field->size(), 1);
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(ContainerFormat, UuidValidSax) {
    auto json = formats::json::MakeObject("my_field", formats::json::MakeArray("01234567-89ab-cdef-0123-456789abcdef"));
    auto obj = CallSaxParser<ns::ContainerWithFormatItem>(ToString(json));
    EXPECT_EQ(obj.my_field->size(), 1);
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(ContainerFormat, UuidInvalid) {
    auto json = formats::json::MakeObject("my_field", formats::json::MakeArray("not-a-uuid"));
#if BOOST_VERSION >= 109100
    UEXPECT_THROW_MSG(
        json.As<ns::ContainerWithFormatItem>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'my_field[0]': Invalid UUID string at position 0: hex digit expected"
    );
#else
    UEXPECT_THROW_MSG(
        json.As<ns::ContainerWithFormatItem>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'my_field[0]': invalid uuid string"
    );
#endif
}

USERVER_NAMESPACE_END

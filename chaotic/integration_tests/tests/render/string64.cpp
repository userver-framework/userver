#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/variant.hpp>

#include <schemas/string64.hpp>
#include <schemas/string64_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, String64) {
    auto str64 = crypto::base64::String64{"hello, userver!"};
    auto obj = ns::ObjectString64{str64};

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["value"].As<std::string>();
    EXPECT_EQ(str, "aGVsbG8sIHVzZXJ2ZXIh");
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
    EXPECT_EQ(TestWriteToStream(obj), TestDomSerializer(obj));

    auto new_obj = formats::json::MakeObject("value", str).As<ns::ObjectString64>();
    EXPECT_EQ(new_obj.value, str64);
}

TEST(Simple, String64Sax) {
    auto json = formats::json::MakeObject("value", "aGVsbG8sIHVzZXJ2ZXIh");
    auto obj = CallSaxParser<ns::ObjectString64>(ToString(json));
    EXPECT_EQ(obj.value, crypto::base64::String64{"hello, userver!"});
    EXPECT_EQ(TestWriteToStream(obj), json);
}

// TODO: add validation test for invalid base64 input once format:byte rejects malformed data
// TEST(Simple, String64Invalid) {
//     auto json = formats::json::MakeObject("value", "!!!");
//     UEXPECT_THROW_MSG(json.As<ns::ObjectString64>(), chaotic::Error<formats::json::Value>, "...");
// }

USERVER_NAMESPACE_END

#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <schemas/object_name.hpp>
#include <schemas/object_name_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, CppName) {
    const ns::ObjectName obj;
    EXPECT_EQ(obj.bar, std::nullopt);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
    EXPECT_EQ(TestWriteToStream(obj), TestDomSerializer(obj));
}

TEST(Simple, CppNameSax) {
    const ns::ObjectName obj;
    auto json = TestDomSerializer(obj);
    auto parsed = CallSaxParser<ns::ObjectName>(ToString(json));
    EXPECT_EQ(parsed.bar, std::nullopt);
    EXPECT_EQ(TestWriteToStream(parsed), json);
}

USERVER_NAMESPACE_END

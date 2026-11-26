#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>

#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include <vector>

#include <schemas/extra_container.hpp>
#include <schemas/extra_container_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, ExtraType) {
    static_assert(std::is_same_v<
                  decltype(std::declval<ns::ObjectWithExtraType>().extra),
                  std::map<std::string, std::string>>);
}

TEST(Simple, EmptyExtraType) {
    const auto json = formats::json::MakeObject();
    const auto object = json.As<ns::ObjectWithExtraType>();
    EXPECT_EQ(formats::json::ValueBuilder{object}.ExtractValue(), json);
}

TEST(Simple, SaxTypedExtrasDoNotLeakBetweenObjects) {
    const auto values = CallSaxParser<
        std::vector<ns::ObjectWithExtraType>>(R"([{"old":"first","same":"first"},{"new":"second","same":"second"},{}])"
    );
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0].extra, (std::map<std::string, std::string>{{"old", "first"}, {"same", "first"}}));
    EXPECT_EQ(values[1].extra, (std::map<std::string, std::string>{{"new", "second"}, {"same", "second"}}));
    EXPECT_TRUE(values[2].extra.empty());
}

TEST(Simple, StreamOptionalFieldOverridesExtraOnlyWhenPresent) {
    ns::ObjectWithExtraType value;
    value.extra = {{"known", "extra-value"}, {"other", "other-value"}};
    EXPECT_EQ(TestWriteToStream(value), TestDomSerializer(value));
    value.known = "declared-value";
    EXPECT_EQ(TestWriteToStream(value), TestDomSerializer(value));
}

USERVER_NAMESPACE_END

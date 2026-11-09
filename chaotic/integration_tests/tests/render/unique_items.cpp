#include <userver/chaotic/validators/unique_items.hpp>

#include <cmath>
#include <initializer_list>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/parser/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/yaml/serialize.hpp>

#include <schemas/unique_items.hpp>
#include <schemas/unique_items_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(UniqueItems, Integers) {
    auto json = formats::json::MakeObject("integers", formats::json::MakeArray(1, 2, 1));
    UEXPECT_THROW_MSG(
        json.As<ns::UniqueItemsObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'integers': Duplicate items are not allowed (1)"
    );
    UEXPECT_THROW_MSG(
        FromJsonString(ToString(json), formats::parse::To<ns::UniqueItemsObject>()),
        formats::json::parser::ParseError,
        "Parse error at pos 18, path 'integers': Error at path 'integers': Duplicate items are not allowed (1)"
    );

    json = formats::json::MakeObject("integers", formats::json::MakeArray(1, 2, 3));
    EXPECT_NO_THROW(json.As<ns::UniqueItemsObject>());
}

TEST(UniqueItems, PreservesOrder) {
    for (const auto* input : {
             R"({"integers":[3,1,2]})",
             R"({"strings":["c","a","b"]})",
             R"({"numbers":[3.5,1.5,2.5]})",
             R"({"booleans":[true,false]})",
             R"({"booleans":[false,true]})",
             R"({"booleans":[false]})",
             R"({"arrays":[[2,1],[1,2],[],[1,1]]})",
             R"({"string_arrays":[["b","a"],["a","b"],[],["a","a"]]})",
             R"({"string_arrays":[]})",
             R"({"string_arrays":[[]]})",
             R"({"nested_arrays":[[[1,2],[3]],[[3],[1,2]],[]]})",
             R"({"integers":[],"strings":[],"numbers":[],"booleans":[],"arrays":[],"nested_arrays":[]})",
             R"({"integers":[1],"strings":["a"],"numbers":[1.5],"booleans":[true],"arrays":[[]],"nested_arrays":[[]]})",
         })
    {
        SCOPED_TRACE(input);
        const auto json = formats::json::FromString(input);
        EXPECT_EQ(TestDomSerializer(json.As<ns::UniqueItemsObject>()), json);
        EXPECT_EQ(TestDomSerializer(CallSaxParser<ns::UniqueItemsObject>(input)), json);
        EXPECT_EQ(TestDomSerializer(formats::yaml::FromString(input).As<ns::UniqueItemsObject>()), json);
    }
}

TEST(UniqueItems, RejectsDuplicates) {
    struct TestCase {
        const char* input;
        std::initializer_list<const char*> duplicates;
    };

    const TestCase kTestCases[] = {
        {R"({"strings":["a","b","a"]})", {"a"}},
        {R"({"numbers":[1.5,2.5,1.5]})", {"1.5"}},
        {R"({"numbers":[1,1.0]})", {"1"}},
        {R"({"numbers":[0.0,-0.0]})", {"0", "-0"}},
        {R"({"integers":[3,1,3,1]})", {"1", "3"}},
        {R"({"strings":["c","a","c","a"]})", {"a", "c"}},
        {R"({"numbers":[3.5,1.5,3.5,1.5]})", {"1.5", "3.5"}},
        {R"({"booleans":[true,true]})", {"true"}},
        {R"({"booleans":[false,false]})", {"false"}},
        {R"({"booleans":[true,false,true]})", {"true"}},
        {R"({"integers":[3,1,3,2]})", {"3"}},
        {R"({"strings":["c","a","c","b"]})", {"c"}},
        {R"({"booleans":[true,false,false]})", {"false"}},
        {R"({"booleans":[true,true,false]})", {"true"}},
        {R"({"numbers":[3.5,1.5,3.5,2.5]})", {"3.5"}},
        {R"({"arrays":[[1,2],[3],[1,2]]})", {}},
        {R"({"arrays":[[],[1],[]]})", {}},
        {R"({"string_arrays":[["a","b"],["c"],["a","b"]]})", {}},
        {R"({"string_arrays":[[],["a"],[]]})", {}},
        {R"({"nested_arrays":[[[1],[2]],[[3]],[[1],[2]]]})", {}},
    };

    for (const auto& [input, duplicates] : kTestCases) {
        SCOPED_TRACE(input);
        std::vector<testing::Matcher<const std::string&>> messages;
        if (duplicates.size() == 0) {
            messages.push_back(testing::EndsWith("Duplicate items are not allowed"));
        }
        for (const auto* duplicate : duplicates) {
            messages.push_back(testing::HasSubstr(fmt::format("Duplicate items are not allowed ({})", duplicate)));
        }
        const auto message = testing::AnyOfArray(messages);
        EXPECT_THAT(
            [&] { formats::json::FromString(input).As<ns::UniqueItemsObject>(); },
            testing::ThrowsMessage<chaotic::Error<formats::json::Value>>(message)
        );
        EXPECT_THAT(
            [&] { CallSaxParser<ns::UniqueItemsObject>(input); },
            testing::ThrowsMessage<formats::json::parser::ParseError>(message)
        );
        EXPECT_THAT(
            [&] { formats::yaml::FromString(input).As<ns::UniqueItemsObject>(); },
            testing::ThrowsMessage<chaotic::Error<formats::yaml::Value>>(message)
        );
    }
}

TEST(UniqueItems, SetContainers) {
    ns::UniqueItemsObject expected;
    expected.integer_set = std::set<int>{1, 2};
    expected.string_unordered_set = std::unordered_set<std::string>{"a", "b"};
    expected.boolean_set = std::set<bool>{false, true};
    for (const auto* input : {
             R"({"integer_set":[2,1],"string_unordered_set":["b","a"],"boolean_set":[true,false]})",
             R"({"integer_set":[2,1,2],"string_unordered_set":["b","a","b"],"boolean_set":[true,false,true]})",
         })
    {
        SCOPED_TRACE(input);
        EXPECT_EQ(formats::json::FromString(input).As<ns::UniqueItemsObject>(), expected);
        EXPECT_EQ(CallSaxParser<ns::UniqueItemsObject>(input), expected);
        EXPECT_EQ(formats::yaml::FromString(input).As<ns::UniqueItemsObject>(), expected);
    }
}

TEST(UniqueItems, YamlAllowsNaNs) {
    const auto only_nans = formats::yaml::FromString("numbers: [.nan, .nan]").As<ns::UniqueItemsObject>();
    ASSERT_TRUE(only_nans.numbers);
    ASSERT_EQ(only_nans.numbers->size(), 2);
    EXPECT_TRUE(std::isnan((*only_nans.numbers)[0]));
    EXPECT_TRUE(std::isnan((*only_nans.numbers)[1]));

    const auto value = formats::yaml::FromString("numbers: [.nan, 1.5, .nan]").As<ns::UniqueItemsObject>();
    ASSERT_TRUE(value.numbers);
    ASSERT_EQ(value.numbers->size(), 3);
    EXPECT_TRUE(std::isnan((*value.numbers)[0]));
    EXPECT_EQ((*value.numbers)[1], 1.5);
    EXPECT_TRUE(std::isnan((*value.numbers)[2]));
}

TEST(UniqueItems, YamlRejectsDuplicatesWithNaN) {
    for (const auto* input : {"numbers: [1.5, .nan, 2.5, 1.5]", "numbers: [0.0, .nan, -0.0]"}) {
        SCOPED_TRACE(input);
        UEXPECT_THROW_MSG(
            formats::yaml::FromString(input).As<ns::UniqueItemsObject>(),
            chaotic::Error<formats::yaml::Value>,
            "Duplicate items are not allowed"
        );
    }
}

USERVER_NAMESPACE_END

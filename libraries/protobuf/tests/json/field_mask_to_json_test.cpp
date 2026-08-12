#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct FieldMaskToJsonSuccessTestParam {
    FieldMaskMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};

    // This variable is used to disable some checks that fail in the native protobuf implementation.
    bool skip_native_check = false;
};

struct FieldMaskToJsonFailureTestParam {
    FieldMaskMessageData input = {};
    PrintErrorCode expected_errc = {};
    std::string expected_path = {};
    PrintOptions options = {};
};

std::vector<std::string> ParseFieldMaskStr(std::string_view paths) {
    std::vector<std::string> result;
    std::size_t pos = 0;

    while (true) {
        auto comma_pos = paths.find(',', pos);

        if (comma_pos == std::string_view::npos) {
            result.emplace_back(paths.substr(pos));
            break;
        }

        result.emplace_back(paths.substr(pos, comma_pos - pos));
        pos = comma_pos + 1;
    }

    return result;
}

namespace {
// fmt::format with fmt/ranges.h cannot be used here: on fmt 8.x (CI sandbox,
// Ubuntu 22.04) fmt/ranges.h calls format_to unqualified inside fmt::detail,
// which causes an ADL ambiguity with std::format_to from libstdc++13 that
// clang-16 rejects as a hard error. Fixed in fmt 9.x (devcontainer), but CI
// still runs 8.1.1.
std::string FmtFormatCrutch(const std::optional<std::vector<std::string>>& items) {
    if (!items) {
        return "nullopt";
    }
    constexpr std::string_view sep = ", ";
    std::string result = "[";
    for (const auto& s : *items) {
        result += '"';
        result += s;
        result += '"';
        result += sep;
    }
    if (!items->empty()) {
        result.resize(result.size() - sep.size());
    }
    result += ']';
    return result;
}
}  // namespace

void PrintTo(const FieldMaskToJsonSuccessTestParam& param, std::ostream* os) {
    *os << "{ input = {.field1=" << FmtFormatCrutch(param.input.field1) << "} }";
}

void PrintTo(const FieldMaskToJsonFailureTestParam& param, std::ostream* os) {
    *os << "{ input = {.field1=" << FmtFormatCrutch(param.input.field1) << "} }";
}

class FieldMaskToJsonSuccessTest : public ::testing::TestWithParam<FieldMaskToJsonSuccessTestParam> {};
class FieldMaskToJsonFailureTest : public ::testing::TestWithParam<FieldMaskToJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    FieldMaskToJsonSuccessTest,
    ::testing::Values(
        FieldMaskToJsonSuccessTestParam{FieldMaskMessageData{}, R"({})"},
        FieldMaskToJsonSuccessTestParam{FieldMaskMessageData{std::vector<std::string>{}}, R"({"field1":""})"},
        FieldMaskToJsonSuccessTestParam{FieldMaskMessageData{std::vector<std::string>{""}}, R"({"field1":""})"},
        FieldMaskToJsonSuccessTestParam{
            FieldMaskMessageData{std::vector<std::string>{"", "", "", "aaa", ""}},
            R"({"field1":",,,aaa,"})",
            {},
            !kIsModernProtoJson
        },
        FieldMaskToJsonSuccessTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some_field"}},
            R"({"field1":"someField"})",
            // Does not affect field mask serialization!
            {.preserve_proto_field_names = true}
        },
        FieldMaskToJsonSuccessTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some_field.another_field..one_more", "_a_b0_c"}},
            R"({"field1":"someField.anotherField..oneMore,AB0C"})",
            {},
            !kIsModernProtoJson
        }
    )
);

// Protobuf ProtoJSON in legacy mode does not return error on invalid characters, which we
// want to prohibit (because we do not want our clients to use syntax that may break in the
// newer protobuf versions).

INSTANTIATE_TEST_SUITE_P(
    ,
    FieldMaskToJsonFailureTest,
    ::testing::Values(
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"Some_field"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some_Field"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some_fielD"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some_f!ield"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"__some_field"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some__field"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some_field_"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        FieldMaskToJsonFailureTestParam{
            FieldMaskMessageData{std::vector<std::string>{"some_0field"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        }
    )
);

TEST_P(FieldMaskToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json;
    formats::json::Value expected_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));

    if (!json.HasMember("field1")) {
        EXPECT_FALSE(expected_json.HasMember("field1"));
        return;
    }

    ASSERT_TRUE(json["field1"].IsString());
    ASSERT_TRUE(expected_json["field1"].IsString());

    auto json_paths = ParseFieldMaskStr(json["field1"].As<std::string>());
    auto expected_json_paths = ParseFieldMaskStr(expected_json["field1"].As<std::string>());

    std::ranges::sort(json_paths);
    std::ranges::sort(expected_json_paths);

    EXPECT_EQ(json_paths, expected_json_paths);

    if (!param.skip_native_check) {
        formats::json::Value sample_json;

        UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

        if (!json.HasMember("field1")) {
            EXPECT_FALSE(sample_json.HasMember("field1"));
            return;
        }

        ASSERT_TRUE(sample_json["field1"].IsString());

        auto sample_json_paths = ParseFieldMaskStr(sample_json["field1"].As<std::string>());
        std::ranges::sort(sample_json_paths);

        EXPECT_EQ(expected_json_paths, sample_json_paths);
    }
}

TEST_P(FieldMaskToJsonFailureTest, Test) {
    const auto& param = GetParam();
    auto input = PrepareTestData(param.input);

    EXPECT_PRINT_ERROR((void)MessageToJson(input, param.options), param.expected_errc, param.expected_path);
}

TEST(FieldMaskToJsonAdditionalTest, InlinedNonNull) {
    FieldMaskMessageData data{std::vector<std::string>{"some_field.nested_field"}};
    auto message = PrepareTestData(data);
    formats::json::Value json;
    formats::json::Value sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsString());
    ASSERT_TRUE(sample.IsString());

    auto paths = ParseFieldMaskStr(json.As<std::string>());
    auto sample_paths = ParseFieldMaskStr(sample.As<std::string>());

    std::ranges::sort(paths);
    std::ranges::sort(sample_paths);

    EXPECT_EQ(paths, sample_paths);
    EXPECT_EQ(paths, std::vector<std::string>{"someField.nestedField"});
}

TEST(FieldMaskToJsonAdditionalTest, InlinedNull) {
    FieldMaskMessageData data{};
    auto message = PrepareTestData(data);
    formats::json::Value json;
    formats::json::Value sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsString());
    ASSERT_TRUE(sample.IsString());
    EXPECT_TRUE(json.As<std::string>().empty());
    EXPECT_TRUE(sample.As<std::string>().empty());
}

TEST(FieldMaskToJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::FieldMask;
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto paths_desc = message->GetDescriptor()->FindFieldByName("paths");

        reflection->AddString(message.get(), paths_desc, "some_field.another_field");
        reflection->AddString(message.get(), paths_desc, "one_more");

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsString());

        auto paths = ParseFieldMaskStr(json.As<std::string>());
        std::ranges::sort(paths);

        ASSERT_EQ(paths.size(), std::size_t{2});
        EXPECT_EQ(paths[0], "oneMore");
        EXPECT_EQ(paths[1], "someField.anotherField");
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END

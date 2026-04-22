#include <userver/formats/yaml/serialize.hpp>

#include <cstdint>
#include <limits>
#include <string>

#include <gtest/gtest.h>

#include <userver/formats/json.hpp>
#include <userver/formats/yaml.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace fy = formats::yaml;
namespace fj = formats::json;

const auto kSimpleJson = fj::MakeObject(
    "string",
    "test",
    "int",
    int64_t{1},
    "double",
    1.1,
    "int64max",
    std::numeric_limits<std::int16_t>::max(),
    "int64min",
    std::numeric_limits<std::int16_t>::min()
);

const std::string kSimpleYaml = R"(
string: test
int: 1
double: 1.1
int64max: 32767
int64min: -32768
)";

const auto kSimpleJsonArray = fj::MakeArray("test", int64_t{1}, 1.0);

const std::string kSimpleYamlArray = R"(
- test
- 1
- 1.0
)";

}  // namespace

TEST(Convert, PrimitiveToJson) {
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(nullptr).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(nullptr).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(false).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(false).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(true).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(true).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(int32_t{2}).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(int32_t{2}).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(int64_t{2}).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(int64_t{2}).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(int64_t{3'000'000}).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(int64_t{3'000'000}).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(1.12).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(1.12).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(1.).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(1.).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fy::ValueBuilder(std::string{"test"}).ExtractValue(), formats::parse::To<fj::Value>{}),
        fj::ValueBuilder(std::string{"test"}).ExtractValue()
    );
}

TEST(Convert, PrimitiveFromJson) {
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(nullptr).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(nullptr).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(false).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(false).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(true).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(true).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(int32_t{2}).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(int32_t{2}).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(int64_t{2}).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(int64_t{2}).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(int64_t{3'000'000}).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(int64_t{3'000'000}).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(uint64_t{3'000'000}).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(uint64_t{3'000'000}).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(1.12).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(1.12).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(1.).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(1.).ExtractValue()
    );
    EXPECT_EQ(
        formats::parse::Parse(fj::ValueBuilder(std::string{"test"}).ExtractValue(), formats::parse::To<fy::Value>{}),
        fy::ValueBuilder(std::string{"test"}).ExtractValue()
    );
}

TEST(Convert, SimpleToJson) {
    auto yaml = fy::FromString(kSimpleYaml);
    auto json = formats::parse::Parse(yaml, formats::parse::To<fj::Value>{});
    EXPECT_EQ(json, kSimpleJson);
}

TEST(Convert, SimpleFromJson) {
    auto yaml = formats::parse::Parse(kSimpleJson, formats::parse::To<fy::Value>{});
    auto json = formats::parse::Parse(yaml, formats::parse::To<fj::Value>{});
    EXPECT_EQ(json, kSimpleJson);
}

TEST(Convert, SimpleToJsonArray) {
    auto yaml = fy::FromString(kSimpleYamlArray);
    auto json = formats::parse::Parse(yaml, formats::parse::To<fj::Value>{});
    EXPECT_EQ(json, kSimpleJsonArray);
}

TEST(Convert, SimpleFromJsonArray) {
    auto yaml = formats::parse::Parse(kSimpleJsonArray, formats::parse::To<fy::Value>{});
    auto json = formats::parse::Parse(yaml, formats::parse::To<fj::Value>{});
    EXPECT_EQ(json, kSimpleJsonArray);
}

TEST(Convert, ToJson) {
    fy::ValueBuilder yb = fy::FromString(kSimpleYaml);
    yb["simple_doc"] = fy::FromString(kSimpleYaml);
    yb["empty_doc"] = fy::ValueBuilder(formats::common::Type::kObject);
    yb["simple_array"] = fy::FromString(kSimpleYamlArray);
    yb["empty_array"] = fy::ValueBuilder(formats::common::Type::kArray);

    auto json = formats::parse::Parse(yb.ExtractValue(), formats::parse::To<fj::Value>{});

    fj::ValueBuilder jb = kSimpleJson;
    jb["simple_doc"] = kSimpleJson;
    jb["empty_doc"] = fj::MakeObject();
    jb["simple_array"] = kSimpleJsonArray;
    jb["empty_array"] = fj::MakeArray();

    EXPECT_EQ(json, jb.ExtractValue());
}

TEST(Convert, FromJson) {
    fj::ValueBuilder jb = kSimpleJson;
    jb["simple_doc"] = kSimpleJson;
    jb["empty_doc"] = fj::MakeObject();
    jb["simple_array"] = kSimpleJsonArray;
    jb["empty_array"] = fj::MakeArray();
    auto expected_json = jb.ExtractValue();

    auto yaml = formats::parse::Parse(expected_json, formats::parse::To<fy::Value>{});
    auto json = formats::parse::Parse(yaml, formats::parse::To<fj::Value>{});

    EXPECT_EQ(json, expected_json);
}

USERVER_NAMESPACE_END

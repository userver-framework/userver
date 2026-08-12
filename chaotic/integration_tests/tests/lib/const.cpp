#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/const_value.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr int kOne = 1;

TEST(Const, IntType) {
    auto value = chaotic::ConstValue<kOne>();

    // test: operator int() const
    [[maybe_unused]] int str = value;

    static_assert(std::is_same_v<decltype(value.GetValue()), int>);
}

TEST(Const, Serialize) {
    auto value = chaotic::ConstValue<kOne>();
    auto value_str = ToString(formats::json::ValueBuilder{value}.ExtractValue());
    EXPECT_EQ(value_str, "1");
}

TEST(Const, WriteToStream) {
    auto value = chaotic::ConstValue<kOne>();

    formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    EXPECT_EQ(builder.GetString(), "1");
}

constexpr utils::StringLiteral kFoo = "foo";

TEST(Const, StringType) {
    auto value = chaotic::ConstValue<kFoo>();

    [[maybe_unused]]
    std::string_view str_view(value);

    [[maybe_unused]]
    std::string str(value);

    static_assert(std::is_same_v<decltype(value.GetValue()), utils::StringLiteral>);
}

}  // namespace

USERVER_NAMESPACE_END

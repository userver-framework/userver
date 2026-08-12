#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/parser/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <schemas/const.hpp>
#include <schemas/const_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

// ---- StringConst ----

TEST(Const, StringConstValid) {
    auto json = formats::json::FromString(R"("active")");
    auto val = json.As<ns::StringConst>();
    EXPECT_EQ(val, ns::StringConst{});
}

TEST(Const, StringConstInvalid) {
    auto json = formats::json::FromString(R"("inactive")");
    UEXPECT_THROW_MSG(
        json.As<ns::StringConst>(),
        chaotic::Error<formats::json::Value>,
        "expected='active', actual='inactive'"
    );
}

TEST(Const, StringConstSerialize) {
    const ns::StringConst val{};
    auto json = TestDomSerializer(val);
    EXPECT_EQ(json.As<std::string>(), "active");
    EXPECT_EQ(TestWriteToStream(val), json);
}

TEST(Const, StringConstSax) {
    auto val = CallSaxParser<ns::StringConst>(R"("active")");
    EXPECT_EQ(val, ns::StringConst{});
    EXPECT_EQ(TestWriteToStream(val), formats::json::FromString(R"("active")"));
    EXPECT_EQ(TestWriteToStream(val), TestDomSerializer(val));
}

// ---- IntConst ----

TEST(Const, IntConstValid) {
    auto json = formats::json::FromString("42");
    auto val = json.As<ns::IntConst>();
    EXPECT_EQ(val, ns::IntConst{});
}

TEST(Const, IntConstInvalid) {
    auto json = formats::json::FromString("99");
    UEXPECT_THROW_MSG(json.As<ns::IntConst>(), chaotic::Error<formats::json::Value>, "expected='42', actual='99'");
}

TEST(Const, IntConstSerialize) {
    const ns::IntConst val{};
    auto json = TestDomSerializer(val);
    EXPECT_EQ(json.As<int>(), 42);
    EXPECT_EQ(TestWriteToStream(val), json);
}

TEST(Const, IntConstSax) {
    auto val = CallSaxParser<ns::IntConst>("42");
    EXPECT_EQ(val, ns::IntConst{});
    EXPECT_EQ(TestWriteToStream(val), formats::json::FromString("42"));
    EXPECT_EQ(TestWriteToStream(val), TestDomSerializer(val));
}

// ---- Int64Const ----

TEST(Const, Int64ConstValid) {
    auto json = formats::json::FromString("9000000000");
    auto val = json.As<ns::Int64Const>();
    EXPECT_EQ(val, ns::Int64Const{});
}

TEST(Const, Int64ConstInvalid) {
    auto json = formats::json::FromString("1");
    UEXPECT_THROW_MSG(
        json.As<ns::Int64Const>(),
        chaotic::Error<formats::json::Value>,
        "expected='9000000000', actual='1'"
    );
}

TEST(Const, Int64ConstSerialize) {
    const ns::Int64Const val{};
    auto json = TestDomSerializer(val);
    EXPECT_EQ(json.As<std::int64_t>(), 9000000000);
    EXPECT_EQ(TestWriteToStream(val), json);
}

TEST(Const, Int64ConstSax) {
    auto val = CallSaxParser<ns::Int64Const>("9000000000");
    EXPECT_EQ(val, ns::Int64Const{});
    EXPECT_EQ(TestWriteToStream(val), TestDomSerializer(val));
}

// ---- BoolConst ----

TEST(Const, BoolConstValid) {
    auto json = formats::json::FromString("true");
    auto val = json.As<ns::BoolConst>();
    EXPECT_EQ(val, ns::BoolConst{});
}

TEST(Const, BoolConstInvalid) {
    auto json = formats::json::FromString("false");
    UEXPECT_THROW_MSG(
        json.As<ns::BoolConst>(),
        chaotic::Error<formats::json::Value>,
        "expected='true', actual='false'"
    );
}

TEST(Const, BoolConstSerialize) {
    const ns::BoolConst val{};
    auto json = TestDomSerializer(val);
    EXPECT_EQ(json.As<bool>(), true);
    EXPECT_EQ(TestWriteToStream(val), json);
}

TEST(Const, BoolConstSax) {
    auto val = CallSaxParser<ns::BoolConst>("true");
    EXPECT_EQ(val, ns::BoolConst{});
    EXPECT_EQ(TestWriteToStream(val), formats::json::FromString("true"));
    EXPECT_EQ(TestWriteToStream(val), TestDomSerializer(val));
}

// ---- StandaloneStringConst (inferred type) ----

TEST(Const, StandaloneStringConstValid) {
    auto json = formats::json::FromString(R"("standalone")");
    auto val = json.As<ns::StandaloneStringConst>();
    EXPECT_EQ(val, ns::StandaloneStringConst{});
}

TEST(Const, StandaloneStringConstInvalid) {
    auto json = formats::json::FromString(R"("other")");
    UEXPECT_THROW_MSG(
        json.As<ns::StandaloneStringConst>(),
        chaotic::Error<formats::json::Value>,
        "expected='standalone', actual='other'"
    );
}

TEST(Const, StandaloneStringConstSerialize) {
    const ns::StandaloneStringConst val{};
    auto json = TestDomSerializer(val);
    EXPECT_EQ(json.As<std::string>(), "standalone");
    EXPECT_EQ(TestWriteToStream(val), json);
}

TEST(Const, StandaloneStringConstSax) {
    auto val = CallSaxParser<ns::StandaloneStringConst>(R"("standalone")");
    EXPECT_EQ(val, ns::StandaloneStringConst{});
    EXPECT_EQ(TestWriteToStream(val), formats::json::FromString(R"("standalone")"));
    EXPECT_EQ(TestWriteToStream(val), TestDomSerializer(val));
}

// ---- ObjectWithConst ----

TEST(Const, ObjectWithConstValid) {
    auto json = formats::json::MakeObject("status", "active", "code", 42, "enabled", true);
    auto obj = json.As<ns::ObjectWithConst>();
    EXPECT_EQ(obj.status, ns::StringConst{});
    EXPECT_EQ(obj.code, ns::IntConst{});
    EXPECT_EQ(obj.enabled, ns::BoolConst{});
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Const, ObjectWithConstSax) {
    auto json = formats::json::MakeObject("status", "active", "code", 42, "enabled", true);
    auto obj = CallSaxParser<ns::ObjectWithConst>(ToString(json));
    EXPECT_EQ(obj.status, ns::StringConst{});
    EXPECT_EQ(obj.code, ns::IntConst{});
    EXPECT_EQ(obj.enabled, ns::BoolConst{});
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Const, ObjectWithConstInvalidStatus) {
    auto json = formats::json::MakeObject("status", "inactive", "code", 42, "enabled", true);
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectWithConst>(),
        chaotic::Error<formats::json::Value>,
        "expected='active', actual='inactive'"
    );
}

TEST(Const, ObjectWithConstFromJsonString) {
    auto obj =
        FromJsonString(R"({"status":"active","code":42,"enabled":true})", formats::parse::To<ns::ObjectWithConst>());
    EXPECT_EQ(obj.status, ns::StringConst{});
}

TEST(Const, ObjectWithConstSerialize) {
    const ns::ObjectWithConst obj{
        ns::StringConst{},
        ns::IntConst{},
        ns::BoolConst{},
    };
    auto json = Serialize(obj, formats::serialize::To<formats::json::Value>());
    EXPECT_EQ(json["status"].As<std::string>(), "active");
    EXPECT_EQ(json["code"].As<int>(), 42);
    EXPECT_EQ(json["enabled"].As<bool>(), true);
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Const, ObjectWithConstOperatorEq) {
    ns::ObjectWithConst a{ns::StringConst{}, ns::IntConst{}, ns::BoolConst{}};
    ns::ObjectWithConst b{ns::StringConst{}, ns::IntConst{}, ns::BoolConst{}};
    EXPECT_EQ(a, b);
}

USERVER_NAMESPACE_END

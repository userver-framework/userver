#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/io/struct_as_json.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

/// [struct_as_json_declare]
struct Preferences {
    bool dark_mode{false};
    int page_size{0};
    std::string locale;
    std::vector<std::string> muted;
};

Preferences Parse(const formats::json::Value& json, formats::parse::To<Preferences>) {
    return Preferences{
        json["dark_mode"].As<bool>(false),
        json["page_size"].As<int>(0),
        json["locale"].As<std::string>({}),
        json["muted"].As<std::vector<std::string>>({}),
    };
}

formats::json::Value Serialize(const Preferences& value, formats::serialize::To<formats::json::Value>) {
    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["dark_mode"] = value.dark_mode;
    builder["page_size"] = value.page_size;
    builder["locale"] = value.locale;
    builder["muted"] = value.muted;
    return builder.ExtractValue();
}
/// [struct_as_json_declare]

bool operator==(const Preferences& lhs, const Preferences& rhs) {
    return lhs.dark_mode == rhs.dark_mode && lhs.page_size == rhs.page_size && lhs.locale == rhs.locale &&
           lhs.muted == rhs.muted;
}

// The control for the opt-in: the same shape, with Parse and Serialize, and no
// `kStoreAsJson`. Without it nothing about this type reaches the driver — which
// is the whole point of the opt-in being a separate declaration rather than
// "has Parse and Serialize" on its own, because a great many types have those
// and are not meant to live in a column.
struct NotOptedIn {
    int n{0};
};

NotOptedIn Parse(const formats::json::Value& json, formats::parse::To<NotOptedIn>) {
    return NotOptedIn{json["n"].As<int>(0)};
}

formats::json::Value Serialize(const NotOptedIn& value, formats::serialize::To<formats::json::Value>) {
    return formats::json::MakeObject("n", value.n);
}

// And the other half of the guard: opted in, but with no json mapping at all.
struct OptedInWithoutJson {
    int n{0};
};

// The same type with a **strict** Parse, to demonstrate what the permissive one
// above is avoiding. `As<T>()` with no default throws when the member is
// absent, and a member is absent in every row written before it existed.
struct Strict {
    bool dark_mode{false};
    int page_size{0};
};

Strict Parse(const formats::json::Value& json, formats::parse::To<Strict>) {
    return Strict{json["dark_mode"].As<bool>(), json["page_size"].As<int>()};
}

formats::json::Value Serialize(const Strict& value, formats::serialize::To<formats::json::Value>) {
    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["dark_mode"] = value.dark_mode;
    builder["page_size"] = value.page_size;
    return builder.ExtractValue();
}

const std::string kCreateSchema = "create schema if not exists __pgtest_saj";
const std::string kDropSchema = "drop schema if exists __pgtest_saj cascade";

}  // namespace

namespace storages::postgres::io {

/// [struct_as_json_opt_in]
template <>
constexpr bool kStoreAsJson<Preferences> = true;
/// [struct_as_json_opt_in]

template <>
constexpr bool kStoreAsJson<OptedInWithoutJson> = true;

template <>
constexpr bool kStoreAsJson<Strict> = true;

}  // namespace storages::postgres::io

namespace {

// The opt-in plus a json mapping is what makes the type a column type.
static_assert(io::traits::kIsMappedToPg<Preferences>);
static_assert(io::traits::IsSpecialMapping<Preferences>::value);

// CONTROL: Parse and Serialize without the opt-in map nothing.
static_assert(!io::traits::kIsMappedToPg<NotOptedIn>);
static_assert(!io::traits::IsSpecialMapping<NotOptedIn>::value);

// CONTROL: the opt-in without a json mapping maps nothing either, so the
// declaration cannot make an unserialisable type look storable.
static_assert(!io::traits::kIsMappedToPg<OptedInWithoutJson>);

Preferences MakePreferences() {
    return Preferences{true, 50, "pt-BR", {"wind", "gear"}};
}

}  // namespace

UTEST_P(PostgreConnection, StructAsJsonRoundTripAsParameter) {
    CheckConnection(GetConn());
    const auto expected = MakePreferences();

    /// [struct_as_json_round_trip]
    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", expected));
    const auto got = res.AsSingleRow<Preferences>();
    /// [struct_as_json_round_trip]

    EXPECT_EQ(expected, got);
}

UTEST_P(PostgreConnection, StructAsJsonBindsAsJsonb) {
    CheckConnection(GetConn());

    // jsonb is the default binding, the same as for formats::json::Value — so a
    // struct lands in a jsonb column with no cast at the call site.
    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select pg_typeof($1)::text", MakePreferences()));
    EXPECT_EQ("jsonb", res.AsSingleRow<std::string>());
}

UTEST_P(PostgreConnection, StructAsJsonThroughAColumn) {
    CheckConnection(GetConn());
    UEXPECT_NO_THROW(GetConn()->Execute(kDropSchema));
    UEXPECT_NO_THROW(GetConn()->Execute(kCreateSchema));
    UEXPECT_NO_THROW(GetConn()->Execute("create table __pgtest_saj.account(id int, prefs jsonb, plain json)"));

    const auto expected = MakePreferences();
    UEXPECT_NO_THROW(
        GetConn()->Execute("insert into __pgtest_saj.account(id, prefs, plain) values ($1, $2, $3::json)", 1, expected, expected)
    );

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select prefs from __pgtest_saj.account where id = $1", 1));
    EXPECT_EQ(expected, res.AsSingleRow<Preferences>());

    // Reading back out of a `json` column works too: the parser is given the
    // document, and the column's storage is Postgres's business.
    UEXPECT_NO_THROW(res = GetConn()->Execute("select plain from __pgtest_saj.account where id = $1", 1));
    EXPECT_EQ(expected, res.AsSingleRow<Preferences>());

    UEXPECT_NO_THROW(GetConn()->Execute(kDropSchema));
}

UTEST_P(PostgreConnection, StructAsJsonOptional) {
    CheckConnection(GetConn());
    const auto expected = MakePreferences();

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", std::optional<Preferences>{expected}));
    EXPECT_EQ(expected, res.AsSingleRow<std::optional<Preferences>>().value());

    // And the absent case beside it, because a null column and a populated one
    // take different paths out of the same parser.
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", std::optional<Preferences>{}));
    EXPECT_FALSE(res.AsSingleRow<std::optional<Preferences>>().has_value());
}

UTEST_P(PostgreConnection, StructAsJsonEmptyAndDefaulted) {
    CheckConnection(GetConn());

    // A default-constructed value round-trips rather than arriving as a null or
    // an empty document: every member is written, so every member comes back.
    const Preferences empty{};
    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", empty));
    EXPECT_EQ(empty, res.AsSingleRow<Preferences>());

    // A document missing a member parses to that member's default, which is the
    // type's own Parse deciding — not the driver.
    UEXPECT_NO_THROW(res = GetConn()->Execute(R"(select '{"locale":"es-419"}'::jsonb)"));
    const auto partial = res.AsSingleRow<Preferences>();
    EXPECT_EQ("es-419", partial.locale);
    EXPECT_EQ(0, partial.page_size);
    EXPECT_FALSE(partial.dark_mode);
}


// A jsonb column has no schema, so a row holds whatever the struct looked like
// when that row was written. These two walks are the ones that decide whether a
// deploy is safe, and they are each other's control.
UTEST_P(PostgreConnection, StructAsJsonReadsWhatAnOlderLayoutWrote) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};

    /// [struct_as_json_older_row]
    // What a build before `muted` and `locale` existed would have left in the
    // column. A permissive Parse reads it; every absent member takes its
    // default.
    UEXPECT_NO_THROW(res = GetConn()->Execute(R"(select '{"dark_mode":true,"page_size":25}'::jsonb)"));
    const auto old_row = res.AsSingleRow<Preferences>();
    EXPECT_TRUE(old_row.dark_mode);
    EXPECT_EQ(25, old_row.page_size);
    EXPECT_EQ("", old_row.locale);
    EXPECT_TRUE(old_row.muted.empty());
    /// [struct_as_json_older_row]
}

UTEST_P(PostgreConnection, StructAsJsonReadsWhatANewerLayoutWrote) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};

    // The other direction, which matters during a rollback or a mixed deploy: a
    // member this build has never heard of is ignored rather than fatal.
    UEXPECT_NO_THROW(res = GetConn()->Execute(
                         R"(select '{"dark_mode":true,"page_size":25,"locale":"ar","theme":"dune","muted":[]}'::jsonb)"
                     ));
    const auto newer_row = res.AsSingleRow<Preferences>();
    EXPECT_TRUE(newer_row.dark_mode);
    EXPECT_EQ("ar", newer_row.locale);
}

UTEST_P(PostgreConnection, StructAsJsonStrictParseFailsOnAnOlderRow) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};

    // The control, and the reason the documentation says to write Parse
    // permissively: the same older row, read by a Parse that demands every
    // member, throws rather than degrading.
    UEXPECT_NO_THROW(res = GetConn()->Execute(R"(select '{"dark_mode":true,"page_size":25}'::jsonb)"));
    UEXPECT_NO_THROW(res.AsSingleRow<Strict>());

    UEXPECT_NO_THROW(res = GetConn()->Execute(R"(select '{"dark_mode":true}'::jsonb)"));
    UEXPECT_THROW(res.AsSingleRow<Strict>(), formats::json::MemberMissingException);

    // And the permissive one survives the identical document, which is what
    // makes this a comparison rather than an assertion about json.
    UEXPECT_NO_THROW(res.AsSingleRow<Preferences>());
}

USERVER_NAMESPACE_END

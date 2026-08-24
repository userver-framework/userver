#include <userver/utest/utest.hpp>

#include <string_view>

#include <storages/postgres/postgres_config.hpp>

#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace pg = storages::postgres;

pg::PoolSettings MakeStaticSettings() {
    return pg::PoolSettings{
        .min_size = 4,
        .max_size = 15,
        .max_queue_size = 200,
        .connecting_limit = 8,
        .connecting_interval_ms = 500,
    };
}

pg::PoolSettingsDynamic ParseDynamic(std::string_view json) {
    return formats::json::FromString(json).As<pg::PoolSettingsDynamic>();
}

pg::PoolSettings MergeFromJson(pg::PoolSettings static_settings, std::string_view json) {
    pg::MergePoolSettings(ParseDynamic(json), static_settings);
    return static_settings;
}

}  // namespace

UTEST(PostgrePoolSettings, DynamicOmitsConnectingFields) {
    const auto dynamic = ParseDynamic(R"({
        "min_pool_size": 4,
        "max_pool_size": 15,
        "max_queue_size": 200
    })");

    EXPECT_FALSE(dynamic.connecting_limit.has_value());
    EXPECT_FALSE(dynamic.connecting_interval_ms.has_value());
}

UTEST(PostgrePoolSettings, DynamicExplicitConnectingLimitZero) {
    const auto dynamic = ParseDynamic(R"({
        "min_pool_size": 4,
        "max_pool_size": 15,
        "max_queue_size": 200,
        "connecting_limit": 0,
        "connecting_interval_ms": 0
    })");

    ASSERT_TRUE(dynamic.connecting_limit.has_value());
    EXPECT_EQ(*dynamic.connecting_limit, 0);
    ASSERT_TRUE(dynamic.connecting_interval_ms.has_value());
    EXPECT_EQ(*dynamic.connecting_interval_ms, 0);
}

UTEST(PostgrePoolSettings, DynamicExplicitConnectingLimit) {
    const auto dynamic = ParseDynamic(R"({
        "min_pool_size": 4,
        "max_pool_size": 15,
        "max_queue_size": 200,
        "connecting_limit": 4,
        "connecting_interval_ms": 200
    })");

    ASSERT_TRUE(dynamic.connecting_limit.has_value());
    EXPECT_EQ(*dynamic.connecting_limit, 4);
    ASSERT_TRUE(dynamic.connecting_interval_ms.has_value());
    EXPECT_EQ(*dynamic.connecting_interval_ms, 200);
}

UTEST(PostgrePoolSettings, MergeKeepsStaticWhenConnectingFieldsOmitted) {
    const auto merged = MergeFromJson(MakeStaticSettings(), R"({
        "min_pool_size": 1,
        "max_pool_size": 12,
        "max_queue_size": 200
    })");

    EXPECT_EQ(merged.min_size, 1);
    EXPECT_EQ(merged.max_size, 12);
    EXPECT_EQ(merged.max_queue_size, 200);
    EXPECT_EQ(merged.connecting_limit, 8);
    EXPECT_EQ(merged.connecting_interval_ms, 500);
}

UTEST(PostgrePoolSettings, MergeAppliesExplicitZero) {
    const auto merged = MergeFromJson(MakeStaticSettings(), R"({
        "min_pool_size": 4,
        "max_pool_size": 15,
        "max_queue_size": 200,
        "connecting_limit": 0,
        "connecting_interval_ms": 0
    })");

    EXPECT_EQ(merged.connecting_limit, 0);
    EXPECT_EQ(merged.connecting_interval_ms, 0);
}

UTEST(PostgrePoolSettings, MergeAppliesExplicitConnectingFields) {
    const auto merged = MergeFromJson(MakeStaticSettings(), R"({
        "min_pool_size": 4,
        "max_pool_size": 15,
        "max_queue_size": 200,
        "connecting_limit": 4,
        "connecting_interval_ms": 200
    })");

    EXPECT_EQ(merged.connecting_limit, 4);
    EXPECT_EQ(merged.connecting_interval_ms, 200);
}

UTEST(PostgrePoolSettings, DefaultDictWithoutConnectingLimitKeepsStatic) {
    const auto dict =
        formats::json::FromString(R"({
        "__default__": {
            "min_pool_size": 4,
            "max_pool_size": 15,
            "max_queue_size": 200
        }
    })")
            .As<dynamic_config::ValueDict<pg::PoolSettingsDynamic>>();

    auto settings = MakeStaticSettings();
    pg::MergePoolSettings(dict.GetOptional("postgresql-eats_checkout_base"), settings);

    EXPECT_EQ(settings.connecting_limit, 8);
    EXPECT_EQ(settings.connecting_interval_ms, 500);
}

UTEST(PostgrePoolSettings, DefaultDictExplicitZeroOverridesStatic) {
    const auto dict =
        formats::json::FromString(R"({
        "__default__": {
            "min_pool_size": 4,
            "max_pool_size": 15,
            "max_queue_size": 200,
            "connecting_limit": 0,
            "connecting_interval_ms": 0
        }
    })")
            .As<dynamic_config::ValueDict<pg::PoolSettingsDynamic>>();

    auto settings = MakeStaticSettings();
    pg::MergePoolSettings(dict.GetOptional("postgresql-eats_checkout_base"), settings);

    EXPECT_EQ(settings.connecting_limit, 0);
    EXPECT_EQ(settings.connecting_interval_ms, 0);
}

USERVER_NAMESPACE_END

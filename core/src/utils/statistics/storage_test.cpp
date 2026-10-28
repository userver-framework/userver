#include <userver/utils/statistics/storage.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/json.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(StatisticsStorage, RegisterWriter) {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder = statistics_storage.RegisterWriter("foo.bar.baz", [](utils::statistics::Writer& writer) {
        writer = 42;
    });

    const auto json = formats::json::FromString(utils::statistics::ToJsonFormat(statistics_storage));
    EXPECT_EQ(json["foo.bar.baz"][0]["value"].As<int>(), 42);
}

USERVER_NAMESPACE_END

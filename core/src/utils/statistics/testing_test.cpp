#include <userver/utils/statistics/testing.hpp>

#include <cstdint>
#include <string>

#include <userver/formats/json/serialize.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/resource_scopes.hpp>
#include <userver/utils/statistics/histogram.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(Snapshot, Printable) {
    const utils::statistics::Storage storage;
    const utils::statistics::Snapshot snapshot{storage};

    EXPECT_TRUE(true) << testing::PrintToString(snapshot);
}

/// [metrics Snapshot sample]
namespace {

class ToyMetrics {
public:
    ToyMetrics(utils::ResourceScopeStorage& scopes, utils::statistics::Storage& storage) {
        storage.RegisterWriter(scopes, "toy", [this](utils::statistics::Writer& writer) {
            writer["foo"] = foo;
            writer["bar"] = bar;
        });
    }

    std::int64_t foo{1};
    std::int64_t bar{2};
};

}  // namespace

UTEST(Snapshot, FromStorage) {
    utils::statistics::Storage storage;
    const utils::WithResourceScopes<ToyMetrics> toy(std::in_place, storage);

    const utils::statistics::Snapshot snapshot{storage};

    EXPECT_EQ(snapshot.SingleMetric("toy.foo"), std::int64_t{1});
    EXPECT_EQ(snapshot.SingleMetric("toy.bar"), std::int64_t{2});
}
/// [metrics Snapshot sample]

UTEST(Snapshot, ToJsonFormatRoundTrip) {
    utils::statistics::Histogram histogram{std::vector<double>{1.5, 5, 42, 60}};
    histogram.Account(1.2);
    histogram.Account(10, 5);
    histogram.Account(100);

    const utils::statistics::Snapshot snapshot{[&histogram](utils::statistics::Writer& writer) {
        writer["handler.timings"] = histogram;
    }};
    const auto json =
        formats::json::FromString(utils::statistics::ToJsonFormat([&snapshot](utils::statistics::Writer& writer) {
            writer = snapshot;
        }));
    EXPECT_EQ(json["handler.timings"][0]["type"].As<std::string>(), "HIST_RATE");
    EXPECT_FALSE(json.HasMember("test"));
    EXPECT_FALSE(json.HasMember("test.handler.timings"));

    const auto loaded = utils::statistics::Snapshot::FromJson(json);
    EXPECT_THAT(loaded, EqualsNumerically(snapshot));
    EXPECT_EQ(loaded.SingleMetric("handler.timings").AsHistogram().GetTotalSum(), histogram.GetView().GetTotalSum());
}

namespace {

struct RoundTripMetrics {
    std::int64_t in_flight{4};
    double duration_seconds{1.5};
    utils::statistics::Rate requests{3};
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const RoundTripMetrics& metrics) {
    writer["in_flight"] = metrics.in_flight;
    writer["duration_seconds"] = metrics.duration_seconds;
    writer["requests"].ValueWithLabels(metrics.requests, {"handler", "/orders"});
}

}  // namespace

UTEST(Snapshot, ToJsonFormatRoundTripLabels) {
    const RoundTripMetrics metrics;
    const utils::statistics::Snapshot snapshot{metrics};
    const auto json =
        formats::json::FromString(utils::statistics::ToJsonFormat([&snapshot](utils::statistics::Writer& writer) {
            writer = snapshot;
        }));

    EXPECT_EQ(json["in_flight"][0]["type"].As<std::string>(), "GAUGE");
    EXPECT_EQ(json["in_flight"][0]["value"].As<std::int64_t>(), 4);
    EXPECT_EQ(json["duration_seconds"][0]["value"].As<double>(), 1.5);
    EXPECT_EQ(json["requests"][0]["type"].As<std::string>(), "RATE");
    EXPECT_FALSE(json.HasMember("test.in_flight"));

    const auto loaded = utils::statistics::Snapshot::FromJson(json);
    EXPECT_THAT(loaded, EqualsNumerically(snapshot));
    EXPECT_EQ(loaded.SingleMetric("in_flight"), std::int64_t{4});
    EXPECT_NE(loaded.SingleMetric("in_flight"), 4.0);
    EXPECT_EQ(loaded.SingleMetric("duration_seconds"), 1.5);
    EXPECT_EQ(loaded.SingleMetric("requests", {{"handler", "/orders"}}), utils::statistics::Rate{3});

    const auto layered = utils::statistics::Snapshot::FromLayeredJson(formats::json::FromString(R"({
        "in_flight": 4,
        "duration_seconds": 1.5,
        "requests": {"handler = /orders": {"type": "RATE", "value": 3}}
    })"));
    const auto layered_json =
        formats::json::FromString(utils::statistics::ToJsonFormat([&layered](utils::statistics::Writer& writer) {
            writer = layered;
        }));
    EXPECT_THAT(utils::statistics::Snapshot::FromJson(layered_json), EqualsNumerically(layered));
}

namespace {

class SnapshotStorageSlicing : public testing::TestWithParam<bool> {};

}  // namespace

UTEST_P(SnapshotStorageSlicing, ToJsonFormatExportsStoredPathsAndLabels) {
    utils::statistics::Storage storage;
    const utils::WithResourceScopes<ToyMetrics> toy(std::in_place, storage);
    auto scope = storage.RegisterWriter("api", [](utils::statistics::Writer& writer) {
        writer["requests"].ValueWithLabels(std::int64_t{3}, {{"service", "orders"}, {"code", "200"}});
    });
    const utils::statistics::Snapshot snapshot{
        storage,
        {
            .require_prefix = "api",
            .require_labels = {{"service", "orders"}},
            .sliced = GetParam(),
        },
    };

    const std::string path = GetParam() ? "requests" : "api.requests";
    const auto json =
        formats::json::FromString(utils::statistics::ToJsonFormat([&snapshot](utils::statistics::Writer& writer) {
            writer = snapshot;
        }));
    ASSERT_TRUE(json.HasMember(path));
    EXPECT_FALSE(json.HasMember(GetParam() ? "api.requests" : "requests"));
    EXPECT_EQ(json[path][0]["labels"]["code"].As<std::string>(), "200");
    EXPECT_EQ(json[path][0]["labels"].HasMember("service"), !GetParam());

    const auto loaded = utils::statistics::Snapshot::FromJson(json);
    EXPECT_THAT(loaded, EqualsNumerically(snapshot));
    const std::vector<utils::statistics::Label> labels =
        GetParam() ? std::vector<utils::statistics::Label>{{"code", "200"}}
                   : std::vector<utils::statistics::Label>{{"service", "orders"}, {"code", "200"}};
    EXPECT_EQ(loaded.SingleMetric(path, labels), std::int64_t{3});
}

INSTANTIATE_UTEST_SUITE_P(Snapshot, SnapshotStorageSlicing, testing::Bool());

USERVER_NAMESPACE_END

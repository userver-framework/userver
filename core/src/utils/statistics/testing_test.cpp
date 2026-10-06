#include <userver/utils/statistics/testing.hpp>

#include <cstdint>

#include <userver/utest/utest.hpp>
#include <userver/utils/resource_scopes.hpp>
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

USERVER_NAMESPACE_END

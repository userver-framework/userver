#include <userver/cache/base_postgres_cache.hpp>

#include <memory>
#include <unordered_map>

#include <userver/tracing/span.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Data = std::unordered_map<int, int>;
using CopyOnWriteData = components::pg_cache::detail::CopyOnWriteData<Data>;

}  // namespace

UTEST(PostgreCacheCopyOnWriteData, NoCopyWithoutNewData) {
    const auto current = std::make_shared<const Data>(Data{{1, 10}});
    const CopyOnWriteData data{current};

    EXPECT_TRUE(data.NeedsCopy());
    EXPECT_EQ(&data.GetLatestData(), current.get());
    EXPECT_EQ(data.GetCopiedSize(), 0);
}

UTEST(PostgreCacheCopyOnWriteData, CopiesOnFirstGetOrCreateNewData) {
    tracing::Span span{"test"};
    auto scope = span.CreateScopeTime();
    const auto current = std::make_shared<const Data>(Data{{1, 10}, {2, 20}});
    CopyOnWriteData data{current};

    auto& new_data = data.GetOrCreateNewData(0, scope);
    EXPECT_FALSE(data.NeedsCopy());
    EXPECT_NE(&new_data, current.get());
    EXPECT_EQ(new_data, *current);
    EXPECT_EQ(data.GetCopiedSize(), 2);

    new_data[3] = 30;
    EXPECT_EQ(&data.GetOrCreateNewData(0, scope), &new_data);
    EXPECT_EQ(&data.GetLatestData(), &new_data);
    EXPECT_EQ(current->size(), 2);

    const auto released = std::move(data).ReleaseNewData();
    EXPECT_EQ(released.get(), &new_data);
    EXPECT_EQ(released->size(), 3);
}

UTEST(PostgreCacheCopyOnWriteData, NewDataWithoutCurrentData) {
    CopyOnWriteData data{nullptr};

    EXPECT_FALSE(data.NeedsCopy());
    EXPECT_TRUE(data.GetLatestData().empty());
    EXPECT_EQ(data.GetCopiedSize(), 0);
    EXPECT_NE(std::move(data).ReleaseNewData(), nullptr);
}

USERVER_NAMESPACE_END

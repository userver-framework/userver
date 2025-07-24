#include <concurrent/impl/striped_intrusive_pool.hpp>

#include <atomic>
#include <cstddef>
#include <thread>
#include <unordered_set>
#include <vector>

#include <gmock/gmock.h>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/irange.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

struct CheckedInt {
    CheckedInt() : CheckedInt(42) {}

    explicit CheckedInt(int x) : x(x) { UASSERT(x != 0); }

    ~CheckedInt() {
        CheckAlive();
        x = 0;
    }

    void CheckAlive() const { EXPECT_NE(x, 0) << "UB detected, possibly use-after-free"; }

    int x;
    concurrent::impl::SinglyLinkedHook<CheckedInt> hook;
};

using CheckedIntPool = concurrent::impl::
    StripedIntrusivePool<CheckedInt, concurrent::impl::MemberHook<&CheckedInt::hook>, offsetof(CheckedInt, hook)>;

constexpr int kAttempts = 100;

}  // namespace

TEST(StripedIntrusivePool, Empty) {
    CheckedIntPool pool;
    ASSERT_FALSE(pool.TryPop());
}

TEST(StripedIntrusivePool, CanHoldSingle) {
    for (int attempt = 0; attempt < kAttempts; ++attempt) {
        CheckedInt node;
        CheckedIntPool pool;

        pool.Push(node);
        // There is a slim chance that the CPU core switches here and we'll get nothing.
        if (auto* popped_node = pool.TryPop()) {
            EXPECT_EQ(popped_node, &node);
            EXPECT_EQ(pool.TryPop(), nullptr);
            return;
        }
    }

    FAIL() << "Either we are very unlucky (thread constantly switches between CPU cores), or there is a bug";
}

TEST(StripedIntrusivePool, WalkUnsafe) {
    const auto node_values = boost::irange(1, 1024);
    CheckedIntPool pool;

    for (const auto value : node_values) {
        pool.Push(*new CheckedInt{value});
        std::this_thread::yield();
    }

    EXPECT_EQ(pool.GetSizeUnsafe(), node_values.size());

    std::unordered_set<int> values;
    boost::copy(node_values, std::inserter(values, values.begin()));
    pool.WalkUnsafe([&](const CheckedInt& node) { EXPECT_EQ(values.erase(node.x), 1); });
    EXPECT_THAT(values, testing::IsEmpty());

    pool.DisposeUnsafe([](CheckedInt& node) { delete &node; });
}

UTEST_MT(StripedIntrusivePool, TortureTest, 12) {
    // 'nodes' must outlive 'stack'
    utils::FixedArray<CheckedInt> nodes(GetThreadCount() * 2);

    constexpr std::size_t kMinNodesPerTask = 2;
    CheckedIntPool pool;

    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<std::size_t>> tasks;
    tasks.reserve(GetThreadCount() - 1);

    for (std::size_t i = 0; i < GetThreadCount() - 1; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&] {
            std::size_t nodes_created = 0;
            std::vector<std::unique_ptr<CheckedInt>> nodes_we_could_pop;

            while (keep_running) {
                while (auto* node = pool.TryPop()) {
                    node->CheckAlive();
                    nodes_we_could_pop.emplace_back(node);
                }

                while (nodes_we_could_pop.size() < kMinNodesPerTask) {
                    ++nodes_created;
                    nodes_we_could_pop.push_back(std::make_unique<CheckedInt>());
                }

                std::this_thread::yield();

                while (!nodes_we_could_pop.empty()) {
                    pool.Push(*nodes_we_could_pop.back().release());
                    nodes_we_could_pop.pop_back();
                }

                std::this_thread::yield();
            }

            return nodes_created;
        }));
    }

    engine::SleepFor(50ms);
    keep_running = false;

    std::size_t total_nodes_created = 0;
    for (auto& task : tasks) {
        total_nodes_created += task.Get();
    }

    EXPECT_EQ(pool.GetSizeUnsafe(), total_nodes_created);
    pool.DisposeUnsafe([](CheckedInt& node) { delete &node; });
}

USERVER_NAMESPACE_END

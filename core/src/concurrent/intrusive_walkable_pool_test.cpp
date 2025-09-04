#include <concurrent/intrusive_walkable_pool.hpp>

#include <atomic>
#include <cstddef>
#include <thread>
#include <unordered_set>
#include <vector>

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

    concurrent::impl::IntrusiveWalkablePoolHook<CheckedInt> pool_hook;
    int x;
};

using CheckedIntPool = concurrent::impl::IntrusiveWalkablePool<
    CheckedInt,
    concurrent::impl::MemberHook<&CheckedInt::pool_hook>,
    offsetof(CheckedInt, pool_hook)>;

}  // namespace

UTEST(IntrusiveWalkablePool, Walk) {
    CheckedIntPool pool;

    CheckedInt& node1 = pool.Acquire();
    node1.CheckAlive();

    CheckedInt& node2 = pool.Acquire();
    node2.CheckAlive();
    EXPECT_NE(&node1, &node2);

    // The node is not actually deleted. It's just marked that the previous owner
    // doesn't need it anymore, and it can be given to someone else later in
    // Acquire.
    pool.Release(node1);
    node1.CheckAlive();

    std::unordered_multiset<CheckedInt*> walked_nodes;
    pool.Walk([&](CheckedInt& node) { walked_nodes.insert(&node); });
    EXPECT_EQ(walked_nodes, (std::unordered_multiset<CheckedInt*>{&node1, &node2}));

    CheckedInt& new_node = pool.Acquire();
    // new_node may be equal to node1, to node2 or be a newly allocated node.

    pool.Release(node2);
    pool.Release(new_node);
}

UTEST_MT(IntrusiveWalkablePool, TortureTest, 4) {
    constexpr std::size_t kNodesPerTask = 3;
    CheckedIntPool pool;

    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(GetThreadCount() - 1);

    for (std::size_t i = 0; i < GetThreadCount() - 2; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&] {
            CheckedInt* nodes[kNodesPerTask]{};

            while (keep_running) {
                for (auto*& node : nodes) {
                    node = &pool.Acquire();
                    node->CheckAlive();
                }

                for (auto*& node : nodes) {
                    pool.Release(*node);
                }
            }
        }));
    }

    tasks.push_back(engine::AsyncNoSpan([&] {
        while (keep_running) {
            std::size_t node_count = 0;
            pool.Walk([&](CheckedInt& node) {
                node.CheckAlive();
                ++node_count;
            });
            EXPECT_LE(node_count, kNodesPerTask * std::thread::hardware_concurrency());
        }
    }));

    engine::SleepFor(50ms);
    keep_running = false;
    for (auto& task : tasks) task.Get();
}

USERVER_NAMESPACE_END

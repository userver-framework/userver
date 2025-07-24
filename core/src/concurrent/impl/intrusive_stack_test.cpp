#include <userver/concurrent/impl/intrusive_stack.hpp>

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

    concurrent::impl::SinglyLinkedHook<CheckedInt> hook;
    int x;
};

using CheckedIntStack = concurrent::impl::IntrusiveStack<CheckedInt, concurrent::impl::MemberHook<&CheckedInt::hook>>;

}  // namespace

TEST(IntrusiveStack, Empty) {
    CheckedIntStack stack;
    ASSERT_FALSE(stack.TryPop());
}

TEST(IntrusiveStack, CanHoldSingle) {
    CheckedInt node;
    CheckedIntStack stack;

    stack.Push(node);
    EXPECT_EQ(stack.TryPop(), &node);
    EXPECT_EQ(stack.TryPop(), nullptr);
}

UTEST_MT(IntrusiveStack, TortureTest, 12) {
    // 'nodes' must outlive 'stack'
    utils::FixedArray<CheckedInt> nodes(GetThreadCount() * 2);

    CheckedIntStack stack;
    for (auto& node : nodes) stack.Push(node);

    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(GetThreadCount() - 1);

    for (std::size_t i = 0; i < GetThreadCount() - 1; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&] {
            std::vector<CheckedInt*> our_nodes;
            our_nodes.reserve(nodes.size());

            while (keep_running) {
                while (auto* node = stack.TryPop()) {
                    node->CheckAlive();
                    our_nodes.push_back(node);
                }

                while (!our_nodes.empty()) {
                    stack.Push(*our_nodes.back());
                    our_nodes.pop_back();
                }

                // ABA is possible if task 1 pushes, pops and pushes again its nodes -
                // all while task 2 is inside a Pop. This way we test
                // IntrusiveStack ABA protection.
                //
                // On practice, if we remove the ABA protection, the test deadlocks.
            }
        }));
    }

    engine::SleepFor(50ms);
    keep_running = false;
    for (auto& task : tasks) task.Get();
}

USERVER_NAMESPACE_END

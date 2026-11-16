#include <ugrpc/impl/event_engine/timer_storage.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

struct Node final {
    std::atomic<bool> acquired{false};
    TimerStorageHook<Node> hook;
};

using Storage = TimerStorage<Node, concurrent::impl::MemberHook<&Node::hook>>;

Node& Acquire(Storage& storage) {
    Node& node = storage.Acquire([] { return Node{}; });
    EXPECT_FALSE(node.acquired.exchange(true)) << "The storage returned a node that is still acquired";
    return node;
}

void Release(Storage& storage, Node& node) {
    EXPECT_TRUE(node.acquired.exchange(false));
    storage.Release(node);
}

std::unordered_set<Node*> AllNodes(Storage& storage) {
    std::unordered_set<Node*> nodes;
    storage.Walk([&](Node& node) { nodes.insert(&node); });
    return nodes;
}

TEST(GrpcEventEngineTimerStorage, NodesReleasedOnAnotherThreadAreReused) {
    constexpr std::size_t kNodes = 64;
    Storage storage;
    std::vector<Node*> nodes;
    for (std::size_t i = 0; i < kNodes; ++i) {
        nodes.push_back(&Acquire(storage));
    }
    std::thread([&] {
        for (Node* const node : nodes) {
            Release(storage, *node);
        }
    }).join();
    std::unordered_set<Node*> reused;
    for (std::size_t i = 0; i < kNodes; ++i) {
        reused.insert(&Acquire(storage));
    }
    EXPECT_EQ(reused, std::unordered_set<Node*>(nodes.begin(), nodes.end()));
    EXPECT_EQ(AllNodes(storage).size(), kNodes);
    for (Node* const node : reused) {
        Release(storage, *node);
    }
}

TEST(GrpcEventEngineTimerStorage, WalkVisitsAcquiredAndReleasedNodes) {
    Storage storage;
    Node& acquired = Acquire(storage);
    Node& released = Acquire(storage);
    Release(storage, released);
    EXPECT_EQ(AllNodes(storage), (std::unordered_set<Node*>{&acquired, &released}));
    Release(storage, acquired);
}

TEST(GrpcEventEngineTimerStorage, NodeCountIsBoundedWhenAnotherThreadReleases) {
    constexpr std::size_t kAcquirers = 4;
    constexpr std::size_t kNodesPerAcquirer = 5'000;
    constexpr std::size_t kLiveNodesToThrottle = 32;
    Storage storage;
    // Includes acquisitions that have not returned yet and releases that have not completed.
    std::atomic<std::size_t> live_nodes{0};
    std::atomic<std::size_t> max_live_nodes{0};
    std::mutex mutex;
    std::vector<Node*> handoff;

    std::thread releaser([&] {
        std::size_t released = 0;
        while (released < kAcquirers * kNodesPerAcquirer) {
            std::vector<Node*> nodes;
            {
                const std::lock_guard lock(mutex);
                nodes.swap(handoff);
            }
            if (nodes.empty()) {
                std::this_thread::yield();
                continue;
            }
            for (Node* const node : nodes) {
                Release(storage, *node);
                --live_nodes;
            }
            released += nodes.size();
        }
    });
    std::vector<std::thread> acquirers;
    for (std::size_t i = 0; i < kAcquirers; ++i) {
        acquirers.emplace_back([&] {
            for (std::size_t j = 0; j < kNodesPerAcquirer; ++j) {
                while (live_nodes.load() >= kLiveNodesToThrottle) {
                    std::this_thread::yield();
                }
                const std::size_t live = ++live_nodes;
                std::size_t max_live = max_live_nodes.load();
                while (max_live < live && !max_live_nodes.compare_exchange_weak(max_live, live)) {
                }
                Node& node = Acquire(storage);
                const std::lock_guard lock(mutex);
                handoff.push_back(&node);
            }
        });
    }
    for (auto& acquirer : acquirers) {
        acquirer.join();
    }
    releaser.join();

    EXPECT_LE(AllNodes(storage).size(), max_live_nodes.load());
}

}  // namespace
}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

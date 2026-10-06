#include <userver/storages/redis/command_control.hpp>

#include <string>
#include <thread>
#include <vector>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(CommandControl, Comparison) {
    const storages::redis::CommandControl cc1{std::chrono::milliseconds(500), std::chrono::milliseconds(500), 1};
    const storages::redis::CommandControl cc2{std::chrono::milliseconds(500), std::chrono::milliseconds(500), 2};
    auto cc3 = cc1;
    cc3.force_request_to_master = true;

    EXPECT_NE(cc1, cc2);
    EXPECT_NE(cc1, cc3);
    EXPECT_EQ(cc1, cc1);
    EXPECT_EQ(cc2, cc2);
    EXPECT_EQ(cc3, cc3);
}

TEST(ServerId, ConcurrentDescriptions) {
    constexpr std::size_t kThreadsCount = 16;
    std::vector<storages::redis::ServerId> ids;
    std::vector<std::string> descriptions;
    ids.reserve(kThreadsCount);
    descriptions.reserve(kThreadsCount);
    for (std::size_t i = 0; i < kThreadsCount; ++i) {
        ids.push_back(storages::redis::ServerId::Generate());
        descriptions.push_back("server-" + std::to_string(i));
    }

    std::vector<std::thread> threads;
    threads.reserve(kThreadsCount);
    for (std::size_t i = 0; i < kThreadsCount; ++i) {
        threads.emplace_back([&, i] { ids[i].SetDescription(descriptions[i]); });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (std::size_t i = 0; i < kThreadsCount; ++i) {
        EXPECT_EQ(ids[i].GetDescription(), descriptions[i]);
    }

    threads.clear();
    for (const auto id : ids) {
        threads.emplace_back([id] { id.RemoveDescription(); });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (const auto id : ids) {
        EXPECT_TRUE(id.GetDescription().empty());
    }
}

USERVER_NAMESPACE_END

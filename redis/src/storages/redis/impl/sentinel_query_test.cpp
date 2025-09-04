#include <gtest/gtest.h>
#include <storages/redis/impl/sentinel_query.hpp>
#include <userver/storages/redis/reply.hpp>

#include <hiredis/hiredis.h>

USERVER_NAMESPACE_BEGIN

TEST(SentinelQuery, SingleBadReply) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(true, storages::redis::Password("pass"), cb, 1);

    auto reply = std::make_shared<storages::redis::Reply>("cmd", storages::redis::ReplyData("str"));
    context->GenerateCallback()(nullptr, reply);

    EXPECT_EQ(0, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, ParseReplySimpleIps) {
    using ReplyData = storages::redis::ReplyData;
    using Array = ReplyData::Array;
    constexpr auto kIp1 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:aaaa";
    constexpr auto kPort1 = 6379;

    constexpr auto kIp2 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:bbbb";
    constexpr auto kPort2 = 6380;

    constexpr auto kIp3 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:cccc";
    constexpr auto kPort3 = 6381;

    constexpr auto kIp4 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:dddd";
    constexpr auto kPort4 = 6382;

    constexpr auto kIp5 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:eeee";
    constexpr auto kPort5 = 6383;

    constexpr auto kIp6 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:ffff";
    constexpr auto kPort6 = 6384;

    auto reply = std::make_shared<storages::redis::Reply>(
        "CLUSTER SLOTS",
        ReplyData{Array{
            Array{
                ReplyData{0},
                ReplyData{5460},
                Array{
                    ReplyData{"klg-9.db.net"},
                    ReplyData{kPort1},
                    ReplyData{"92f260b22c5d1d2b1da1971c0244d268b3aaaaaa"},
                    Array{ReplyData{"ip"}, ReplyData{kIp1}},
                },
                Array{
                    ReplyData{"vla-i.db.net"},
                    ReplyData{kPort2},
                    ReplyData{"4732a74a0c4c9fe245eb8098ff6c2b1b22bbbbbb"},
                    Array{ReplyData{"ip"}, ReplyData{kIp2}},
                },
            },
            Array{
                ReplyData{5461},
                ReplyData{10922},
                Array{
                    ReplyData{"klg-8.db.net"},
                    ReplyData{kPort3},
                    ReplyData{"294e10240d74f7d7eb9e8583645f08f3bdcccccc"},
                    Array{ReplyData{"ip"}, ReplyData{kIp3}},
                },
                Array{
                    ReplyData{"vla-3.db.net"},
                    ReplyData{kPort4},
                    ReplyData{"4732a74a0c4c9fe245eb8098ff6c2b1b22dddddd"},
                    Array{ReplyData{"ip"}, ReplyData{kIp4}},
                },
            },
            Array{
                ReplyData{10923},
                ReplyData{16383},
                Array{
                    ReplyData{"klg-g.db.net"},
                    ReplyData{kPort5},
                    ReplyData{"294e10240d74f7d7eb9e8583645f08f3bd000000"},
                    Array{ReplyData{"ip"}, ReplyData{kIp5}},
                },
                Array{
                    ReplyData{"vla-e.db.net"},
                    ReplyData{kPort6},
                    ReplyData{"4732a74a0c4c9fe245eb8098ff6c2b1b22111111"},
                    Array{ReplyData{"ip"}, ReplyData{kIp6}},
                },
            },
        }}
    );

    namespace impl = storages::redis::impl;
    impl::ClusterSlotsResponse response;
    ASSERT_EQ(impl::ParseClusterSlotsResponse(reply, response), impl::ClusterSlotsResponseStatus::kOk);

    using Pair = std::pair<std::string, int>;
    EXPECT_EQ(response.size(), 3);

    EXPECT_EQ(response[impl::SlotInterval(0, 5460)].master.HostPort(), Pair(kIp1, kPort1));
    ASSERT_EQ(response[impl::SlotInterval(0, 5460)].slaves.size(), 1);
    EXPECT_EQ(response[impl::SlotInterval(0, 5460)].slaves.begin()->HostPort(), Pair(kIp2, kPort2));

    EXPECT_EQ(response[impl::SlotInterval(5461, 10922)].master.HostPort(), Pair(kIp3, kPort3));
    ASSERT_EQ(response[impl::SlotInterval(5461, 10922)].slaves.size(), 1);
    EXPECT_EQ(response[impl::SlotInterval(5461, 10922)].slaves.begin()->HostPort(), Pair(kIp4, kPort4));

    EXPECT_EQ(response[impl::SlotInterval(10923, 16383)].master.HostPort(), Pair(kIp5, kPort5));
    ASSERT_EQ(response[impl::SlotInterval(10923, 16383)].slaves.size(), 1);
    EXPECT_EQ(response[impl::SlotInterval(10923, 16383)].slaves.begin()->HostPort(), Pair(kIp6, kPort6));
}

TEST(SentinelQuery, ParseReplySimpleHostname) {
    using ReplyData = storages::redis::ReplyData;
    using Array = ReplyData::Array;
    constexpr auto kIp1 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:aaaa";
    constexpr auto kPort1 = 6379;

    constexpr auto kIp2 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:bbbb";
    constexpr auto kPort2 = 6380;

    constexpr auto kIp3 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:cccc";
    constexpr auto kPort3 = 6381;

    constexpr auto kIp4 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:dddd";
    constexpr auto kPort4 = 6382;

    constexpr auto kIp5 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:eeee";
    constexpr auto kPort5 = 6383;

    constexpr auto kIp6 = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:ffff";
    constexpr auto kPort6 = 6384;

    auto reply = std::make_shared<storages::redis::Reply>(
        "CLUSTER SLOTS",
        ReplyData{Array{
            Array{
                ReplyData{0},
                ReplyData{5460},
                Array{
                    ReplyData{kIp1},
                    ReplyData{kPort1},
                    ReplyData{"92f260b22c5d1d2b1da1971c0244d268b3aaaaaa"},
                    Array{ReplyData{"hostname"}, ReplyData{"klg-9.db.net"}},
                },
                Array{
                    ReplyData{kIp2},
                    ReplyData{kPort2},
                    ReplyData{"4732a74a0c4c9fe245eb8098ff6c2b1b22bbbbbb"},
                    Array{ReplyData{"hostname"}, ReplyData{"vla-i.db.net"}},
                },
            },
            Array{
                ReplyData{5461},
                ReplyData{16383},
                Array{
                    ReplyData{kIp3},
                    ReplyData{kPort3},
                    ReplyData{"294e10240d74f7d7eb9e8583645f08f3bdcccccc"},
                    Array{ReplyData{"hostname"}, ReplyData{"klg-8.db.net"}},
                },
                Array{
                    ReplyData{kIp4},
                    ReplyData{kPort4},
                    ReplyData{"4732a74a0c4c9fe245eb8098ff6c2b1b22dddddd"},
                    Array{ReplyData{"hostname"}, ReplyData{"vla-3.db.net"}},
                },
                Array{
                    ReplyData{kIp5},
                    ReplyData{kPort5},
                    ReplyData{"294e10240d74f7d7eb9e8583645f08f3bd000000"},
                    Array{ReplyData{"hostname"}, ReplyData{"klg-g.db.net"}},
                },
                Array{
                    ReplyData{kIp6},
                    ReplyData{kPort6},
                    ReplyData{"4732a74a0c4c9fe245eb8098ff6c2b1b22111111"},
                    Array{ReplyData{"hostname"}, ReplyData{"vla-e.db.net"}},
                },
            },
        }}
    );

    namespace impl = storages::redis::impl;
    impl::ClusterSlotsResponse response;
    ASSERT_EQ(impl::ParseClusterSlotsResponse(reply, response), impl::ClusterSlotsResponseStatus::kOk);

    using Pair = std::pair<std::string, int>;
    EXPECT_EQ(response.size(), 2);

    EXPECT_EQ(response[impl::SlotInterval(0, 5460)].master.HostPort(), Pair(kIp1, kPort1));
    ASSERT_EQ(response[impl::SlotInterval(0, 5460)].slaves.size(), 1);
    EXPECT_EQ(response[impl::SlotInterval(0, 5460)].slaves.begin()->HostPort(), Pair(kIp2, kPort2));

    EXPECT_EQ(response[impl::SlotInterval(5461, 16383)].master.HostPort(), Pair(kIp3, kPort3));
    ASSERT_EQ(response[impl::SlotInterval(5461, 16383)].slaves.size(), 3);

    std::set<Pair> host_ports = {
        {kIp4, kPort4},
        {kIp5, kPort5},
        {kIp6, kPort6},
    };
    for (const auto& slave : response[impl::SlotInterval(5461, 16383)].slaves) {
        EXPECT_TRUE(host_ports.extract(slave.HostPort()));
    }
}

std::shared_ptr<storages::redis::Reply>
GenerateReply(const std::string& ip, bool master, bool s_down, bool o_down, bool master_link_status_err) {
    std::string flags = master ? "master" : "slave";
    if (s_down) flags += ",s_down";
    if (o_down) flags += ",o_down";

    std::vector<storages::redis::ReplyData> array{
        {"flags"}, {std::move(flags)}, {"name"}, {"inst-name"}, {"ip"}, {ip}, {"port"}, {"1111"}};
    if (!master) {
        array.emplace_back("master-link-status");
        array.emplace_back(master_link_status_err ? "err" : "ok");
    }

    std::vector<storages::redis::ReplyData> array2{storages::redis::ReplyData(std::move(array))};
    return std::make_shared<storages::redis::Reply>("cmd", storages::redis::ReplyData(std::move(array2)));
}

const auto kHost1 = "127.0.0.1";
const auto kHost2 = "127.0.0.2";

TEST(SentinelQuery, SingleOkReply) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(true, storages::redis::Password("pass"), cb, 1);

    auto reply = GenerateReply(kHost1, false, false, false, false);
    context->GenerateCallback()(nullptr, reply);

    EXPECT_EQ(1, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, SingleSDownReply) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(true, storages::redis::Password("pass"), cb, 1);

    auto reply = GenerateReply(kHost2, false, true, false, false);
    context->GenerateCallback()(nullptr, reply);

    EXPECT_EQ(0, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkOk) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost1, false, false, false, false);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(1, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkMastererr) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost1, false, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    reply = GenerateReply(kHost1, false, false, false, true);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(1, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkMastererrMastererr) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost1, false, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    reply = GenerateReply(kHost1, false, false, false, true);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkSDown) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost1, false, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    reply = GenerateReply(kHost1, false, true, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(1, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkSDownSDown) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost1, false, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    reply = GenerateReply(kHost1, false, true, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkODown) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;
    int size = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        size = info.size();
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost1, false, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, called);

    reply = GenerateReply(kHost1, false, false, true, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(0, size);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, DifferentAnswers1) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        ASSERT_EQ(info.size(), 1u);
        const auto& shard_info = info[0];
        EXPECT_EQ(shard_info.HostPort().first, kHost1);
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost1, true, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    context->GenerateCallback()(nullptr, reply);
    reply = GenerateReply(kHost2, true, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(1, called);
}

TEST(SentinelQuery, DifferentAnswers2) {
    const storages::redis::impl::SentinelInstanceResponse resp;

    int called = 0;

    auto cb = [&](const storages::redis::impl::ConnInfoByShard& info, size_t, size_t) {
        called++;
        ASSERT_EQ(info.size(), 1u);
        const auto& shard_info = info[0];
        EXPECT_EQ(shard_info.HostPort().first, kHost1);
    };
    auto context =
        std::make_shared<storages::redis::impl::GetHostsContext>(1, storages::redis::Password("pass"), cb, 3);

    auto reply = GenerateReply(kHost2, true, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    reply = GenerateReply(kHost1, true, false, false, false);
    context->GenerateCallback()(nullptr, reply);
    context->GenerateCallback()(nullptr, reply);
    EXPECT_EQ(1, called);
}

USERVER_NAMESPACE_END

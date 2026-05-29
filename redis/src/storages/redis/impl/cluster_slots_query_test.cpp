#include <gtest/gtest.h>
#include <storages/redis/impl/cluster_slots_query.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

TEST(ClusterSlotsQuery, ParseReplySimpleIps) {
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
    ASSERT_EQ(impl::ParseClusterSlotsResponse(reply, response, "redisdb"), impl::ClusterSlotsResponseStatus::kOk);

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

TEST(ClusterSlotsQuery, ParseReplySimpleHostname) {
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
    ASSERT_EQ(impl::ParseClusterSlotsResponse(reply, response, "redisdb"), impl::ClusterSlotsResponseStatus::kOk);

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

USERVER_NAMESPACE_END

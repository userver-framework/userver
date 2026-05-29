#include <gtest/gtest.h>
#include <storages/redis/impl/cluster_shards_query.hpp>
#include <storages/redis/impl/cluster_slots_query.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ServerId = storages::redis::ServerId;
using ClusterShardsHost = storages::redis::impl::ClusterShardsHost;
using ClusterShardsShard = storages::redis::impl::ClusterShardsShard;
using ClusterShardsResponseStatus = storages::redis::impl::ClusterShardsResponseStatus;
using ClusterShardsResponse = storages::redis::impl::ClusterShardsResponse;
using SlotInterval = storages::redis::impl::SlotInterval;
using ClusterShardsResponsesByServerId =
    std::unordered_map<ServerId, ClusterShardsResponse, storages::redis::ServerIdHasher>;
using ReplyData = storages::redis::ReplyData;
using Reply = storages::redis::Reply;
using ReplyPtr = storages::redis::ReplyPtr;
using ReplyStatus = storages::redis::ReplyStatus;

ClusterShardsHost CreateHost(const std::string& ip, uint16_t port, bool is_master, int64_t replication_offset = 0) {
    ClusterShardsHost host;
    host.ip = ip;
    host.port = port;
    host.is_master = is_master;
    host.replication_offset = replication_offset;
    return host;
}

ClusterShardsShard CreateShard(
    const std::vector<ClusterShardsHost>& hosts,
    const std::vector<SlotInterval>& slot_intervals
) {
    ClusterShardsShard shard;
    shard.hosts = hosts;
    shard.slot_intervals = slot_intervals;
    return shard;
}

ClusterShardsResponse CreateClusterShardsResponse(const std::vector<ClusterShardsShard>& shards) { return shards; }

}  // namespace

TEST(ConvertToClusterSlotsResponse, EmptyResponse) {
    ClusterShardsResponsesByServerId responses_by_id;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    EXPECT_TRUE(result.empty());
}

TEST(ConvertToClusterSlotsResponse, SingleServerSingleShard) {
    const std::string master_ip = "192.168.1.1";
    const std::string slave_ip = "192.168.1.2";
    const auto master_port = 6379;
    const auto slave_port = 6380;

    ClusterShardsResponse shards_response = CreateClusterShardsResponse({CreateShard(
        {CreateHost(master_ip, master_port, true), CreateHost(slave_ip, slave_port, false)},
        {SlotInterval(0, 5460)}
    )});

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.count(server_id), 1);

    const auto& slots_response = result.at(server_id);
    ASSERT_EQ(slots_response.size(), 1);

    const auto& [interval, conn_infos] = *slots_response.begin();
    EXPECT_EQ(interval.slot_min, 0);
    EXPECT_EQ(interval.slot_max, 5460);
    EXPECT_EQ(conn_infos.master.HostPort(), std::make_pair(master_ip, static_cast<int>(master_port)));
    ASSERT_EQ(conn_infos.slaves.size(), 1);
    EXPECT_EQ(conn_infos.slaves.begin()->HostPort(), std::make_pair(slave_ip, static_cast<int>(slave_port)));
}

TEST(ConvertToClusterSlotsResponse, SingleServerMultipleShards) {
    const std::string master_ip1 = "192.168.1.1";
    const std::string slave_ip1 = "192.168.1.2";
    const std::string master_ip2 = "192.168.1.3";
    const std::string slave_ip2 = "192.168.1.4";
    const std::string master_ip3 = "192.168.1.5";
    const std::string slave_ip3 = "192.168.1.6";

    ClusterShardsResponse shards_response = CreateClusterShardsResponse(
        {CreateShard({CreateHost(master_ip1, 6379, true), CreateHost(slave_ip1, 6380, false)}, {SlotInterval(0, 5460)}),
         CreateShard(
             {CreateHost(master_ip2, 6379, true), CreateHost(slave_ip2, 6380, false)},
             {SlotInterval(5461, 10922)}
         ),
         CreateShard(
             {CreateHost(master_ip3, 6379, true), CreateHost(slave_ip3, 6380, false)},
             {SlotInterval(10923, 16383)}
         )}
    );

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    ASSERT_EQ(slots_response.size(), 3);

    // Check first shard
    auto it = slots_response.find(SlotInterval(0, 5460));
    ASSERT_NE(it, slots_response.end());
    EXPECT_EQ(it->second.master.HostPort(), std::make_pair(master_ip1, 6379));
    ASSERT_EQ(it->second.slaves.size(), 1);
    EXPECT_EQ(it->second.slaves.begin()->HostPort(), std::make_pair(slave_ip1, 6380));

    // Check second shard
    it = slots_response.find(SlotInterval(5461, 10922));
    ASSERT_NE(it, slots_response.end());
    EXPECT_EQ(it->second.master.HostPort(), std::make_pair(master_ip2, 6379));
    ASSERT_EQ(it->second.slaves.size(), 1);
    EXPECT_EQ(it->second.slaves.begin()->HostPort(), std::make_pair(slave_ip2, 6380));

    // Check third shard
    it = slots_response.find(SlotInterval(10923, 16383));
    ASSERT_NE(it, slots_response.end());
    EXPECT_EQ(it->second.master.HostPort(), std::make_pair(master_ip3, 6379));
    ASSERT_EQ(it->second.slaves.size(), 1);
    EXPECT_EQ(it->second.slaves.begin()->HostPort(), std::make_pair(slave_ip3, 6380));
}

TEST(ConvertToClusterSlotsResponse, ShardWithoutMasterIsSkipped) {
    const std::string slave_ip = "192.168.1.2";

    ClusterShardsResponse shards_response =
        CreateClusterShardsResponse({CreateShard({CreateHost(slave_ip, 6380, false)}, {SlotInterval(0, 5460)})});

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    EXPECT_TRUE(slots_response.empty());
}

TEST(ConvertToClusterSlotsResponse, MultipleServers) {
    const std::string master_ip1 = "192.168.1.1";
    const std::string master_ip2 = "192.168.2.1";

    ClusterShardsResponse shards_response1 =
        CreateClusterShardsResponse({CreateShard({CreateHost(master_ip1, 6379, true)}, {SlotInterval(0, 8191)})});

    ClusterShardsResponse shards_response2 =
        CreateClusterShardsResponse({CreateShard({CreateHost(master_ip2, 6379, true)}, {SlotInterval(8192, 16383)})});

    auto server_id1 = ServerId::Generate();
    auto server_id2 = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id1] = shards_response1;
    responses_by_id[server_id2] = shards_response2;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 2);

    // Check first server
    ASSERT_EQ(result.count(server_id1), 1);
    const auto& slots_response1 = result.at(server_id1);
    ASSERT_EQ(slots_response1.size(), 1);
    const auto& [interval1, conn_infos1] = *slots_response1.begin();
    EXPECT_EQ(interval1.slot_min, 0);
    EXPECT_EQ(interval1.slot_max, 8191);
    EXPECT_EQ(conn_infos1.master.HostPort(), std::make_pair(master_ip1, 6379));

    // Check second server
    ASSERT_EQ(result.count(server_id2), 1);
    const auto& slots_response2 = result.at(server_id2);
    ASSERT_EQ(slots_response2.size(), 1);
    const auto& [interval2, conn_infos2] = *slots_response2.begin();
    EXPECT_EQ(interval2.slot_min, 8192);
    EXPECT_EQ(interval2.slot_max, 16383);
    EXPECT_EQ(conn_infos2.master.HostPort(), std::make_pair(master_ip2, 6379));
}

TEST(ConvertToClusterSlotsResponse, ShardWithMultipleSlaves) {
    const std::string master_ip = "192.168.1.1";
    const std::string slave_ip1 = "192.168.1.2";
    const std::string slave_ip2 = "192.168.1.3";
    const std::string slave_ip3 = "192.168.1.4";

    ClusterShardsResponse shards_response = CreateClusterShardsResponse({CreateShard(
        {CreateHost(master_ip, 6379, true),
         CreateHost(slave_ip1, 6380, false),
         CreateHost(slave_ip2, 6381, false),
         CreateHost(slave_ip3, 6382, false)},
        {SlotInterval(0, 16383)}
    )});

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    ASSERT_EQ(slots_response.size(), 1);

    const auto& [interval, conn_infos] = *slots_response.begin();
    EXPECT_EQ(conn_infos.master.HostPort(), std::make_pair(master_ip, 6379));
    ASSERT_EQ(conn_infos.slaves.size(), 3);

    std::set<std::pair<std::string, int>> expected_slaves = {{slave_ip1, 6380}, {slave_ip2, 6381}, {slave_ip3, 6382}};

    for (const auto& slave : conn_infos.slaves) {
        EXPECT_TRUE(expected_slaves.erase(slave.HostPort()));
    }
    EXPECT_TRUE(expected_slaves.empty());
}

TEST(ConvertToClusterSlotsResponse, ShardWithMultipleSlotIntervals) {
    const std::string master_ip = "192.168.1.1";
    const std::string slave_ip = "192.168.1.2";

    ClusterShardsResponse shards_response = CreateClusterShardsResponse({CreateShard(
        {CreateHost(master_ip, 6379, true), CreateHost(slave_ip, 6380, false)},
        {SlotInterval(0, 1000), SlotInterval(2000, 3000), SlotInterval(5000, 6000)}
    )});

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    ASSERT_EQ(slots_response.size(), 3);

    // All intervals should map to the same master/slaves
    for (const auto& [interval, conn_infos] : slots_response) {
        EXPECT_EQ(conn_infos.master.HostPort(), std::make_pair(master_ip, 6379));
        ASSERT_EQ(conn_infos.slaves.size(), 1);
        EXPECT_EQ(conn_infos.slaves.begin()->HostPort(), std::make_pair(slave_ip, 6380));
    }

    // Verify all intervals are present
    EXPECT_NE(slots_response.find(SlotInterval(0, 1000)), slots_response.end());
    EXPECT_NE(slots_response.find(SlotInterval(2000, 3000)), slots_response.end());
    EXPECT_NE(slots_response.find(SlotInterval(5000, 6000)), slots_response.end());
}

TEST(ConvertToClusterSlotsResponse, MixedValidAndInvalidShards) {
    const std::string master_ip1 = "192.168.1.1";
    const std::string slave_ip1 = "192.168.1.2";
    const std::string slave_ip2 = "192.168.1.3";
    const std::string master_ip2 = "192.168.1.4";
    const std::string slave_ip3 = "192.168.1.5";

    ClusterShardsResponse shards_response = CreateClusterShardsResponse(
        {// Valid shard with master
         CreateShard({CreateHost(master_ip1, 6379, true), CreateHost(slave_ip1, 6380, false)}, {SlotInterval(0, 5460)}),
         // Invalid shard - no master
         CreateShard({CreateHost(slave_ip2, 6380, false)}, {SlotInterval(5461, 10922)}),
         // Valid shard with master
         CreateShard(
             {CreateHost(master_ip2, 6379, true), CreateHost(slave_ip3, 6380, false)},
             {SlotInterval(10923, 16383)}
         )
        }
    );

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);

    // Only valid shards should be present
    ASSERT_EQ(slots_response.size(), 2);
    EXPECT_NE(slots_response.find(SlotInterval(0, 5460)), slots_response.end());
    EXPECT_EQ(slots_response.find(SlotInterval(5461, 10922)), slots_response.end());
    EXPECT_NE(slots_response.find(SlotInterval(10923, 16383)), slots_response.end());
}

TEST(ConvertToClusterSlotsResponse, ShardWithOnlyMaster) {
    const std::string master_ip = "192.168.1.1";

    ClusterShardsResponse shards_response =
        CreateClusterShardsResponse({CreateShard({CreateHost(master_ip, 6379, true)}, {SlotInterval(0, 16383)})});

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    ASSERT_EQ(slots_response.size(), 1);

    const auto& [interval, conn_infos] = *slots_response.begin();
    EXPECT_EQ(conn_infos.master.HostPort(), std::make_pair(master_ip, 6379));
    EXPECT_TRUE(conn_infos.slaves.empty());
}

TEST(ConvertToClusterSlotsResponse, EmptyShardsList) {
    ClusterShardsResponse shards_response;

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    EXPECT_TRUE(slots_response.empty());
}

TEST(ConvertToClusterSlotsResponse, IPv6Addresses) {
    const std::string master_ip = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:aaaa";
    const std::string slave_ip = "2a02:6b8:c2d:3d21:7a01:1405:4c4f:bbbb";

    ClusterShardsResponse shards_response = CreateClusterShardsResponse(
        {CreateShard({CreateHost(master_ip, 6379, true), CreateHost(slave_ip, 6380, false)}, {SlotInterval(0, 16383)})}
    );

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    ASSERT_EQ(slots_response.size(), 1);

    const auto& [interval, conn_infos] = *slots_response.begin();
    EXPECT_EQ(conn_infos.master.HostPort(), std::make_pair(master_ip, 6379));
    ASSERT_EQ(conn_infos.slaves.size(), 1);
    EXPECT_EQ(conn_infos.slaves.begin()->HostPort(), std::make_pair(slave_ip, 6380));
}

TEST(ConvertToClusterSlotsResponse, DifferentPorts) {
    const std::string master_ip = "192.168.1.1";
    const std::string slave_ip = "192.168.1.2";
    constexpr auto master_port = 7000;
    constexpr auto slave_port = 7001;

    ClusterShardsResponse shards_response = CreateClusterShardsResponse({CreateShard(
        {CreateHost(master_ip, master_port, true), CreateHost(slave_ip, slave_port, false)},
        {SlotInterval(0, 16383)}
    )});

    auto server_id = ServerId::Generate();
    ClusterShardsResponsesByServerId responses_by_id;
    responses_by_id[server_id] = shards_response;

    auto result = ConvertToClusterSlotsResponse(responses_by_id);

    ASSERT_EQ(result.size(), 1);
    const auto& slots_response = result.at(server_id);
    ASSERT_EQ(slots_response.size(), 1);

    const auto& [interval, conn_infos] = *slots_response.begin();
    EXPECT_EQ(conn_infos.master.HostPort(), std::make_pair(master_ip, master_port));
    ASSERT_EQ(conn_infos.slaves.size(), 1);
    EXPECT_EQ(conn_infos.slaves.begin()->HostPort(), std::make_pair(slave_ip, slave_port));
}

// Helper function to create a ReplyPtr for testing
ReplyPtr CreateReply(ReplyData&& data, ReplyStatus status = ReplyStatus::kOk) {
    return std::make_shared<Reply>("CLUSTER SHARDS", std::move(data), status);
}

TEST(ParseClusterShardsResponse, ValidSingleShard) {
    // Create a valid CLUSTER SHARDS response with one shard
    auto response_data = ReplyData(ReplyData::Array{
        // Shard
        ReplyData(ReplyData::Array{
            ReplyData("slots"),
            ReplyData(ReplyData::Array{
                ReplyData(0),  //
                ReplyData(5460)
            }),
            ReplyData("nodes"),
            ReplyData(ReplyData::Array{
                ReplyData(ReplyData::Array{
                    ReplyData("ip"),
                    ReplyData("192.168.1.1"),
                    ReplyData("port"),
                    ReplyData(6379),
                    ReplyData("role"),
                    ReplyData("master"),
                    ReplyData("health"),
                    ReplyData("online"),
                    ReplyData("replication-offset"),
                    ReplyData(123456789)
                }),
                ReplyData(ReplyData::Array{
                    ReplyData("ip"),
                    ReplyData("192.168.1.2"),
                    ReplyData("port"),
                    ReplyData(6380),
                    ReplyData("role"),
                    ReplyData("slave"),
                    ReplyData("health"),
                    ReplyData("online")
                })
            })
        })
    });
    auto reply = CreateReply(std::move(response_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    ASSERT_EQ(result.size(), 1);

    const auto& shard = result[0];
    ASSERT_EQ(shard.hosts.size(), 2);
    EXPECT_EQ(shard.hosts[0].ip, "192.168.1.1");
    EXPECT_EQ(shard.hosts[0].port, 6379);
    EXPECT_EQ(shard.hosts[0].replication_offset, 123456789);
    EXPECT_TRUE(shard.hosts[0].is_master);
    EXPECT_EQ(shard.hosts[1].ip, "192.168.1.2");
    EXPECT_EQ(shard.hosts[1].port, 6380);
    EXPECT_FALSE(shard.hosts[1].is_master);

    ASSERT_EQ(shard.slot_intervals.size(), 1);
    EXPECT_EQ(shard.slot_intervals[0].slot_min, 0);
    EXPECT_EQ(shard.slot_intervals[0].slot_max, 5460);
}

TEST(ParseClusterShardsResponse, ValidMultipleShards) {
    // Create a valid CLUSTER SHARDS response with multiple shards
    ReplyData response_data = ReplyData::Array(
        {// Shards
         ReplyData::Array{
             ReplyData("slots"),
             ReplyData(ReplyData::Array{ReplyData(0), ReplyData(5460)}),
             ReplyData("nodes"),
             ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
                 ReplyData("ip"),
                 ReplyData("192.168.1.1"),
                 ReplyData("port"),
                 ReplyData(6379),
                 ReplyData("health"),
                 ReplyData("online"),
                 ReplyData("role"),
                 ReplyData("master")
             })})
         },
         ReplyData::Array{
             ReplyData("slots"),
             ReplyData(ReplyData::Array{ReplyData(5461), ReplyData(10922)}),
             ReplyData("nodes"),
             ReplyData(ReplyData::Array{
                 // Nodes
                 ReplyData(ReplyData::Array{
                     ReplyData("ip"),
                     ReplyData("192.168.1.2"),
                     ReplyData("port"),
                     ReplyData(6379),
                     ReplyData("health"),
                     ReplyData("online"),
                     ReplyData("role"),
                     ReplyData("master")
                 })
             })
         }
        }
    );
    auto reply = CreateReply(std::move(response_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    ASSERT_EQ(result.size(), 2);

    EXPECT_EQ(result[0].slot_intervals[0].slot_min, 0);
    EXPECT_EQ(result[0].slot_intervals[0].slot_max, 5460);
    EXPECT_EQ(result[0].hosts[0].ip, "192.168.1.1");

    EXPECT_EQ(result[1].slot_intervals[0].slot_min, 5461);
    EXPECT_EQ(result[1].slot_intervals[0].slot_max, 10922);
    EXPECT_EQ(result[1].hosts[0].ip, "192.168.1.2");
}

TEST(ParseClusterShardsResponse, SingleShardWithOneFailedMaster) {
    // Create a valid CLUSTER SHARDS response with one shard
    // one of the instance was master before and failed.
    auto response_data = ReplyData(ReplyData::Array{
        // Shard
        ReplyData(ReplyData::Array{
            ReplyData("slots"),
            ReplyData(ReplyData::Array{
                ReplyData(0),  //
                ReplyData(5460)
            }),
            ReplyData("nodes"),
            ReplyData(ReplyData::Array{
                ReplyData(ReplyData::Array{
                    ReplyData("ip"),
                    ReplyData("192.168.1.1"),
                    ReplyData("port"),
                    ReplyData(6379),
                    ReplyData("role"),
                    ReplyData("master"),
                    ReplyData("health"),
                    ReplyData("failed"),
                    ReplyData("replication-offset"),
                    ReplyData(123456789)
                }),
                ReplyData(ReplyData::Array{
                    ReplyData("ip"),
                    ReplyData("192.168.1.2"),
                    ReplyData("port"),
                    ReplyData(6380),
                    ReplyData("role"),
                    ReplyData("master"),
                    ReplyData("health"),
                    ReplyData("online")
                })
            })
        })
    });
    auto reply = CreateReply(std::move(response_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    ASSERT_EQ(result.size(), 1);

    const auto& shard = result[0];
    ASSERT_EQ(shard.hosts.size(), 2);
    EXPECT_EQ(shard.hosts[0].ip, "192.168.1.1");
    EXPECT_EQ(shard.hosts[0].port, 6379);
    EXPECT_EQ(shard.hosts[0].replication_offset, 123456789);
    // Not a master because it has "failed" health
    EXPECT_FALSE(shard.hosts[0].is_master);
    EXPECT_EQ(shard.hosts[1].ip, "192.168.1.2");
    EXPECT_EQ(shard.hosts[1].port, 6380);
    // Is a master because it has "online" health
    EXPECT_TRUE(shard.hosts[1].is_master);

    ASSERT_EQ(shard.slot_intervals.size(), 1);
    EXPECT_EQ(shard.slot_intervals[0].slot_min, 0);
    EXPECT_EQ(shard.slot_intervals[0].slot_max, 5460);
}

TEST(ParseClusterShardsResponse, UnknownCommandError) {
    // Simulate unknown command error (non-cluster mode)
    auto reply = CreateReply(ReplyData::CreateError("ERR unknown command `CLUSTER`"), ReplyStatus::kOtherError);

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kNonCluster);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, NotOkReply) {
    // Simulate a non-ok reply
    auto reply = CreateReply(ReplyData::CreateError("Some error"), ReplyStatus::kOtherError);

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, NonArrayResponse) {
    // Response is not an array
    auto reply = CreateReply(ReplyData("OK"));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, MissingRequiredHostFields) {
    // Host missing required 'ip' field
    auto reply_data = ReplyData(ReplyData::Array{
        // Shard
        ReplyData(ReplyData::Array{
            ReplyData("slots"),
            ReplyData(ReplyData::Array{ReplyData(0), ReplyData(5460)}),
            ReplyData("nodes"),
            ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
                ReplyData("port"),
                ReplyData(6379),
                ReplyData("role"),
                ReplyData("master"),
                ReplyData("health"),
                ReplyData("online"),
                // Missing 'ip' field
            })})
        })
    });
    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, MissingRequiredShardFields) {
    // Shard missing required 'slots' field
    auto reply_data = ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
        ReplyData("nodes"),
        ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
            ReplyData("ip"),
            ReplyData("192.168.1.1"),
            ReplyData("port"),
            ReplyData(6379),
            ReplyData("role"),
            ReplyData("master"),
            ReplyData("health"),
            ReplyData("online")
        })})
        // Missing 'slots' field
    })});
    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, InvalidSlotRange) {
    // Invalid slot range (max < min)
    auto reply_data = ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
        ReplyData("slots"),
        ReplyData(ReplyData::Array{ReplyData(5460), ReplyData(0)}),
        ReplyData("nodes"),
        ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
            ReplyData("ip"),
            ReplyData("192.168.1.1"),
            ReplyData("port"),
            ReplyData(6379),
            ReplyData("role"),
            ReplyData("master"),
            ReplyData("health"),
            ReplyData("online")
        })})
    })});
    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, SlotOutOfRange) {
    // Slot value out of valid range (0-16384)
    auto reply_data = ReplyData(ReplyData::Array{
        // Shard
        ReplyData(ReplyData::Array{
            ReplyData("slots"),
            ReplyData(ReplyData::Array{ReplyData(0), ReplyData(20000)}),
            ReplyData("nodes"),
            ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
                ReplyData("ip"),
                ReplyData("192.168.1.1"),
                ReplyData("port"),
                ReplyData(6379),
                ReplyData("role"),
                ReplyData("master"),
                ReplyData("health"),
                ReplyData("online")
            })})
        })
    });
    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, OddNumberOfHostElements) {
    // Host map with odd number of elements (missing value)
    auto reply_data = ReplyData(ReplyData::Array{
        // Shard
        ReplyData(ReplyData::Array{
            ReplyData("slots"),
            ReplyData(ReplyData::Array{ReplyData(0), ReplyData(5460)}),
            ReplyData("nodes"),
            ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
                ReplyData("ip"),
                ReplyData("192.168.1.1"),
                ReplyData("port"),
            })})
        })
    });
    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, OddNumberOfSlotElements) {
    // Slots array with odd number of elements (missing max value)
    auto reply_data = ReplyData(ReplyData::Array{
        // Shard
        ReplyData(ReplyData::Array{
            ReplyData("slots"),
            ReplyData(ReplyData::Array{ReplyData(0)}),
            ReplyData("nodes"),
            ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
                ReplyData("ip"),
                ReplyData("192.168.1.1"),
                ReplyData("port"),
                ReplyData(6379),
                ReplyData("role"),
                ReplyData("master"),
            })})
        })
    });
    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, InvalidHostFieldType) {
    // Host field with wrong type (port is string instead of int)
    auto reply_data = ReplyData(ReplyData::Array{
        // Shard
        ReplyData(ReplyData::Array{
            ReplyData("slots"),
            ReplyData(ReplyData::Array{ReplyData(0), ReplyData(5460)}),
            ReplyData("nodes"),
            ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
                ReplyData("ip"),
                ReplyData("192.168.1.1"),
                ReplyData("port"),
                ReplyData("6379"),  // Should be int
                ReplyData("role"),
                ReplyData("master"),
            })})
        })
    });
    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kFail);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, HostWithHostname) {
    // Host with optional hostname field
    ReplyData response_data(ReplyData::Array{ReplyData(ReplyData::Array{
        ReplyData("slots"),
        ReplyData(ReplyData::Array{ReplyData(0), ReplyData(5460)}),
        ReplyData("nodes"),
        ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
            ReplyData("ip"),
            ReplyData("192.168.1.1"),
            ReplyData("hostname"),
            ReplyData("redis-master.example.com"),
            ReplyData("port"),
            ReplyData(6379),
            ReplyData("role"),
            ReplyData("master"),
            ReplyData("health"),
            ReplyData("online")
        })})
    })});
    auto reply = CreateReply(std::move(response_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].hosts.size(), 1);
    EXPECT_EQ(result[0].hosts[0].ip, "192.168.1.1");
    ASSERT_TRUE(result[0].hosts[0].hostname.has_value());
    EXPECT_EQ(result[0].hosts[0].hostname.value(), "redis-master.example.com");
}

TEST(ParseClusterShardsResponse, MultipleSlotIntervals) {
    // Shard with multiple slot intervals
    auto response_data = ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
        ReplyData("slots"),
        ReplyData(ReplyData::Array{
            ReplyData(0),
            ReplyData(1000),
            ReplyData(2000),
            ReplyData(3000),
            ReplyData(5000),
            ReplyData(6000)
        }),
        ReplyData("nodes"),
        ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
            ReplyData("ip"),
            ReplyData("192.168.1.1"),
            ReplyData("port"),
            ReplyData(6379),
            ReplyData("role"),
            ReplyData("master"),
            ReplyData("health"),
            ReplyData("online")
        })})
    })});
    auto reply = CreateReply(std::move(response_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].slot_intervals.size(), 3);
    EXPECT_EQ(result[0].slot_intervals[0].slot_min, 0);
    EXPECT_EQ(result[0].slot_intervals[0].slot_max, 1000);
    EXPECT_EQ(result[0].slot_intervals[1].slot_min, 2000);
    EXPECT_EQ(result[0].slot_intervals[1].slot_max, 3000);
    EXPECT_EQ(result[0].slot_intervals[2].slot_min, 5000);
    EXPECT_EQ(result[0].slot_intervals[2].slot_max, 6000);
}

TEST(ParseClusterShardsResponse, EmptyShardsArray) {
    // Empty shards array
    ReplyData response_data(ReplyData::Array{});
    auto reply = CreateReply(std::move(response_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    EXPECT_TRUE(result.empty());
}

TEST(ParseClusterShardsResponse, ShardWithMultipleNodes) {
    // Shard with multiple nodes (master + multiple slaves)
    auto reply_data = ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
        ReplyData("slots"),
        ReplyData(ReplyData::Array{ReplyData(0), ReplyData(16383)}),
        ReplyData("nodes"),
        ReplyData(ReplyData::Array{
            ReplyData(ReplyData::Array{
                ReplyData("ip"),
                ReplyData("192.168.1.1"),
                ReplyData("port"),
                ReplyData(6379),
                ReplyData("role"),
                ReplyData("master"),
                ReplyData("health"),
                ReplyData("online")
            }),
            ReplyData(ReplyData::Array{
                ReplyData("ip"),
                ReplyData("192.168.1.2"),
                ReplyData("port"),
                ReplyData(6380),
                ReplyData("role"),
                ReplyData("slave"),
                ReplyData("health"),
                ReplyData("online")
            }),
            ReplyData(ReplyData::Array{
                ReplyData("ip"),
                ReplyData("192.168.1.3"),
                ReplyData("port"),
                ReplyData(6381),
                ReplyData("role"),
                ReplyData("slave"),
                ReplyData("health"),
                ReplyData("online")
            })
        })
    })});

    auto reply = CreateReply(std::move(reply_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].hosts.size(), 3);
    EXPECT_TRUE(result[0].hosts[0].is_master);
    EXPECT_FALSE(result[0].hosts[1].is_master);
    EXPECT_FALSE(result[0].hosts[2].is_master);
}

TEST(ParseClusterShardsResponse, UnknownKeysIgnored) {
    // Unknown keys in host and shard should be ignored
    auto response_data = ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
        ReplyData("slots"),
        ReplyData(ReplyData::Array{ReplyData(0), ReplyData(5460)}),
        ReplyData("nodes"),
        ReplyData(ReplyData::Array{ReplyData(ReplyData::Array{
            ReplyData("ip"),
            ReplyData("192.168.1.1"),
            ReplyData("port"),
            ReplyData(6379),
            ReplyData("role"),
            ReplyData("master"),
            ReplyData("unknown-field"),
            ReplyData("some-value")  // Should be ignored
        })}),
        ReplyData("id"),
        ReplyData("shard-123")  // Unknown shard key, should be ignored
    })});
    auto reply = CreateReply(std::move(response_data));

    ClusterShardsResponse result;
    auto status = ParseClusterShardsResponse(reply, result, "test_shard_group");

    EXPECT_EQ(status, ClusterShardsResponseStatus::kOk);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].hosts.size(), 1);
    EXPECT_EQ(result[0].hosts[0].ip, "192.168.1.1");
}

USERVER_NAMESPACE_END

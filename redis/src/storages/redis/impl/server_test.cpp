#include "mock_server_test.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/signals2/connection.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/health_check_param.hpp>

#include <storages/redis/impl/cmd_args.hpp>
#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/connection_kind.hpp>
#include <storages/redis/impl/redis_connection_holder.hpp>
#include <storages/redis/impl/redis_group.hpp>
#include <storages/redis/impl/secdist_redis.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscription_storage.hpp>
#include <storages/redis/impl/thread_pools.hpp>
#include <storages/redis/subscribe_client_impl.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/storages/redis/subscribe_client.hpp>
#include <userver/storages/redis/subscription_token.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// kSuccessTimeout is used only for waits that are expected to succeed
// (EXPECT_TRUE(...WaitForFirstReply...)). WaitForFirstReply returns as soon as
// the awaited reply arrives, so a generous timeout does not slow down the happy
// path while preventing flaps under CI/sanitizer load (the redis ping interval
// alone is 2000ms). Checks that something must NOT happen use kWaitPeriod.
constexpr auto kSuccessTimeout = utest::kMaxTestWaitTime;
constexpr std::chrono::milliseconds kWaitPeriod{10};
constexpr auto kWaitRetries = 100;
constexpr auto kCheckCount = 10;
constexpr auto kRedisDatabaseIndex = 46;
constexpr std::size_t kDatabaseIndex = 0;

const std::string kDbName = "redis_db";
const std::string kLocalhost = "127.0.0.1";
const std::string kPendingUnsubscribeChannel = "pending-unsubscribe";

class RedisServerForConnectionLifecycle final : public MockRedisServer {
public:
    RedisServerForConnectionLifecycle()
        : MockRedisServer(kDbName)
    {}

    ~RedisServerForConnectionLifecycle() override { Stop(); }

protected:
    void OnCommand(ConnectionPtr connection, const storages::redis::Reply& command) override {
        const auto& name = command.data.GetArray().front().GetString();
        if (name == "PING") {
            SendReplyOk(std::move(connection), "PONG");
        } else if (name == "AUTH") {
            SendReplyOk(std::move(connection), "OK");
        } else {
            MockRedisServer::OnCommand(std::move(connection), command);
        }
    }
};

class RedisServerWithPendingUnsubscribe : public MockRedisServer {
public:
    RedisServerWithPendingUnsubscribe()
        : MockRedisServer(kDbName)
    {
        using storages::redis::ReplyData;
        RegisterPingHandler();
        RegisterHandlerWithConstReply(
            "SUBSCRIBE",
            {kPendingUnsubscribeChannel},
            ReplyData::Array{ReplyData("subscribe"), ReplyData(kPendingUnsubscribeChannel), ReplyData(1)}
        );
        const std::string ping_channel{storages::redis::impl::CmdWithArgs::kSubscriberPingChannelName};
        RegisterHandlerWithConstReply(
            "SUBSCRIBE",
            {ping_channel},
            ReplyData::Array{ReplyData("subscribe"), ReplyData(ping_channel), ReplyData(2)}
        );
    }

    ~RedisServerWithPendingUnsubscribe() override { Stop(); }

    bool WaitForUnsubscribe() { return unsubscribe_received_.WaitForEventFor(kSuccessTimeout); }
    bool WaitForHeartbeatUnsubscribe() { return heartbeat_unsubscribe_received_.WaitForEventFor(kSuccessTimeout); }
    bool WaitForDisconnect() { return disconnected_.WaitForEventFor(kSuccessTimeout); }

protected:
    void OnCommand(ConnectionPtr connection, const storages::redis::Reply& command) override {
        const auto& args = command.data.GetArray();
        if (args.front().GetString() != "UNSUBSCRIBE") {
            MockRedisServer::OnCommand(std::move(connection), command);
            return;
        }
        ASSERT_EQ(args.size(), 2);
        if (args[1].GetString() == kPendingUnsubscribeChannel) {
            unsubscribe_received_.Send();
        } else {
            EXPECT_EQ(args[1].GetString(), storages::redis::impl::CmdWithArgs::kSubscriberPingChannelName);
            heartbeat_unsubscribe_received_.Send();
        }
    }

    void OnDisconnected(ConnectionPtr) override { disconnected_.Send(); }

private:
    engine::SingleConsumerEvent unsubscribe_received_;
    engine::SingleConsumerEvent heartbeat_unsubscribe_received_;
    engine::SingleConsumerEvent disconnected_;
};

class RedisServerWithReplicationMonitoring final : public MockRedisServer {
public:
    RedisServerWithReplicationMonitoring()
        : MockRedisServer(kDbName)
    {
        RegisterPingHandler();
        RegisterHandlerWithConstReply("INFO", {"REPLICATION"}, std::string{"master_sync_in_progress:1\r\n"});
    }

    ~RedisServerWithReplicationMonitoring() override { Stop(); }

    bool WaitForInfoRequests(size_t count) {
        std::unique_lock lock{mutex_};
        return info_received_.WaitFor(lock, kSuccessTimeout, [&] { return info_requests_ >= count; });
    }

    size_t GetInfoConnectionCount() const {
        std::lock_guard lock{mutex_};
        return info_connections_.size();
    }

    void ResetInfoRequests() {
        std::lock_guard lock{mutex_};
        info_connections_.clear();
        info_requests_ = 0;
    }

protected:
    void OnCommand(ConnectionPtr connection, const storages::redis::Reply& command) override {
        if (command.data.GetArray().front().GetString() == "INFO") {
            std::lock_guard lock{mutex_};
            info_connections_.insert(connection.get());
            ++info_requests_;
            info_received_.NotifyAll();
        }
        MockRedisServer::OnCommand(std::move(connection), command);
    }

private:
    mutable engine::Mutex mutex_;
    engine::ConditionVariable info_received_;
    std::unordered_set<const Connection*> info_connections_;
    size_t info_requests_{0};
};

template <typename Predicate>
void PeriodicCheck(Predicate predicate) {
    for (int i = 0; i < kCheckCount; i++) {
        EXPECT_TRUE(predicate());
        std::this_thread::sleep_for(kWaitPeriod);
    }
}

template <typename Predicate>
void PeriodicWait(Predicate predicate) {
    for (int i = 0; i < kWaitRetries; i++) {
        if (predicate()) {
            break;
        }
        std::this_thread::sleep_for(kWaitPeriod);
    }
    EXPECT_TRUE(predicate());
}

bool IsConnected(const storages::redis::impl::Redis& redis) {
    return redis.GetState() == storages::redis::RedisState::kConnected;
}

void RegisterSubscriptionHandlers(MockRedisServer& server, const std::string& channel) {
    using storages::redis::ReplyData;
    server.RegisterHandlerWithConstReply(
        "SUBSCRIBE",
        {channel},
        ReplyData::Array{ReplyData("subscribe"), ReplyData(channel), ReplyData(1)}
    );
    server.RegisterHandlerWithConstReply(
        "UNSUBSCRIBE",
        {channel},
        ReplyData::Array{ReplyData("unsubscribe"), ReplyData(channel), ReplyData(0)}
    );
}

void CheckSubscriptionsBypassBuffering(
    storages::redis::impl::RedisConnectionHolder& holder,
    const MockRedisServer::Handler& get_handler,
    const std::string& channel,
    storages::redis::impl::ConnectionMode mode
) {
    using namespace storages::redis;
    const bool has_command_connection = impl::HasCommands(mode);
    struct Replies {
        engine::SingleConsumerEvent subscribed;
        engine::SingleConsumerEvent unsubscribed;
        engine::SingleConsumerEvent first_get;
        engine::SingleConsumerEvent second_get;
        std::atomic_bool valid{true};
    };
    const auto replies = std::make_shared<Replies>();
    const auto before_gets = get_handler.GetReplyCount();
    if (has_command_connection) {
        ASSERT_TRUE(holder.GetConnection(impl::ConnectionKind::kCommand)
                        ->AsyncCommand(impl::PrepareCommand(
                            {"GET", "first"},
                            [replies](const auto&, const auto& reply) {
                                if (!reply->IsOk() || !reply->data.IsString() || reply->data.GetString() != "value") {
                                    replies->valid = false;
                                }
                                replies->first_get.Send();
                            }
                        )));
    }
    ASSERT_TRUE(holder.GetConnection(impl::ConnectionKind::kSubscription)
                    ->AsyncCommand(impl::PrepareCommand(
                        {"SUBSCRIBE", channel},
                        [replies](const auto&, const auto& reply) {
                            if (!reply->IsOk() || !reply->data.IsArray()) {
                                replies->valid = false;
                                return;
                            }
                            const auto& kind = reply->data.GetArray().front().GetString();
                            if (kind == "subscribe") {
                                replies->subscribed.Send();
                            } else if (kind == "unsubscribe") {
                                replies->unsubscribed.Send();
                            }
                        }
                    )));
    ASSERT_TRUE(replies->subscribed.WaitForEventFor(kSuccessTimeout));
    // The ordinary connection still waits for the second command to reach its threshold.
    EXPECT_EQ(get_handler.GetReplyCount(), before_gets);
    if (has_command_connection) {
        ASSERT_TRUE(holder.GetConnection(impl::ConnectionKind::kCommand)
                        ->AsyncCommand(impl::PrepareCommand(
                            {"GET", "second"},
                            [replies](const auto&, const auto& reply) {
                                if (!reply->IsOk() || !reply->data.IsString() || reply->data.GetString() != "value") {
                                    replies->valid = false;
                                }
                                replies->second_get.Send();
                            }
                        )));
        ASSERT_TRUE(replies->first_get.WaitForEventFor(kSuccessTimeout));
        ASSERT_TRUE(replies->second_get.WaitForEventFor(kSuccessTimeout));
    }
    ASSERT_TRUE(holder.GetConnection(impl::ConnectionKind::kSubscription)
                    ->AsyncCommand(impl::PrepareCommand({"UNSUBSCRIBE", channel}, impl::ReplyCallback{})));
    ASSERT_TRUE(replies->unsubscribed.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(replies->valid.load());
    EXPECT_EQ(get_handler.GetReplyCount(), before_gets + (has_command_connection ? 2 : 0));
}

void CheckSubscriptionRouting(
    storages::redis::impl::Sentinel& sentinel,
    const std::string& channel,
    storages::redis::CommandControl cc,
    storages::redis::ServerId& selected_server
) {
    using namespace storages::redis;
    struct Replies {
        engine::SingleConsumerEvent subscribed;
        engine::SingleConsumerEvent unsubscribed;
        ServerId server_id;
        std::atomic_bool valid{true};
    };
    const auto replies = std::make_shared<Replies>();
    sentinel.AsyncCommand(
        impl::PrepareCommand(
            {"SUBSCRIBE", channel},
            [replies](const auto&, const auto& reply) {
                if (!reply->IsOk() || !reply->data.IsArray()) {
                    replies->valid = false;
                    return;
                }
                const auto& kind = reply->data.GetArray().front().GetString();
                if (kind == "subscribe") {
                    replies->server_id = reply->server_id;
                    replies->subscribed.Send();
                } else if (kind == "unsubscribe") {
                    replies->unsubscribed.Send();
                }
            },
            cc
        ),
        false,
        0
    );
    ASSERT_TRUE(replies->subscribed.WaitForEventFor(kSuccessTimeout));
    if (cc.force_server_id) {
        EXPECT_EQ(replies->server_id, *cc.force_server_id);
    }
    selected_server = replies->server_id;
    cc.force_server_id = replies->server_id;
    sentinel.AsyncCommand(impl::PrepareCommand({"UNSUBSCRIBE", channel}, impl::ReplyCallback{}, cc), false, 0);
    ASSERT_TRUE(replies->unsubscribed.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(replies->valid.load());
}

struct MockSentinelServers {
    static constexpr size_t kRedisThreadCount = 1;
    static constexpr std::string_view kRedisName = "redis_name";

    void RegisterSentinelMastersSlaves() {
        std::vector<MockRedisServer::SlaveInfo> slave_infos;
        std::string redis_name{kRedisName};
        for (const auto& slave : slaves) {
            slave_infos.emplace_back(redis_name, kLocalhost, slave.GetPort());
        }

        for (auto& sentinel : sentinels) {
            sentinel.RegisterSentinelMastersHandler({{redis_name, kLocalhost, masters[0].GetPort()}});
            sentinel.RegisterSentinelSlavesHandler(redis_name, slave_infos);
        }
    }

    template <class Function>
    void ForEachServer(const Function& visitor) {
        for (auto& server : masters) {
            visitor(server);
        }
        for (auto& server : slaves) {
            visitor(server);
        }
        for (auto& server : sentinels) {
            visitor(server);
        }
    }

    void CreateSentinelClientAndWait(const secdist::RedisSettings& settings) const {
        auto sentinel_client = storages::redis::impl::Sentinel::CreateSentinel(
            thread_pool,
            settings,
            "test_shard_group_name",
            dynamic_config::GetDefaultSource(),
            storages::redis::impl::SentinelStaticConfig{
                "test_client_name",
                {storages::redis::ShardingStrategy::kKeyShardTaximeterCrc32},
                {},
                {},
            }
        );
        sentinel_client->WaitConnectedDebug(std::empty(slaves));
    }

    void CreateSubscribeSentinelClientAndWait(const secdist::RedisSettings& settings) {
        // Sentinels do NOT receive SUBSCRIBE
        std::vector<MockRedisServer::HandlerPtr> subscribe_handlers;
        for (auto& server : masters) {
            subscribe_handlers.push_back(server.RegisterHandlerWithConstReply("SUBSCRIBE", 1));
        }
        for (auto& server : slaves) {
            subscribe_handlers.push_back(server.RegisterHandlerWithConstReply("SUBSCRIBE", 1));
        }

        auto dynconf = dynamic_config::GetDefaultSource();
        storages::redis::impl::SubscribeRedisGroup group;
        group.db = "test_client_name";
        auto subscribe_sentinel = storages::redis::impl::Sentinel::CreateSentinel(
            thread_pool,
            settings,
            "test_shard_group_name",
            dynconf,
            storages::redis::impl::MakeSentinelStaticConfig(group)
        );
        subscribe_sentinel
            ->WaitConnectedDebug(std::empty(slaves), storages::redis::impl::ConnectionKind::kSubscription);

        storages::redis::SubscribeClientImpl client{std::move(subscribe_sentinel)};

        storages::redis::SubscriptionToken::OnMessageCb callback =
            [](const std::string& channel, const std::string& message) {
                EXPECT_TRUE(false) << "Should not be called. Channel = " << channel << ", message = " << message;
            };
        auto subscription = client.Subscribe("channel_name", std::move(callback), {});

        for (auto& handler : subscribe_handlers) {
            EXPECT_TRUE(handler->WaitForFirstReply(utest::kMaxTestWaitTime));
        }
    }

    MockRedisServer masters[1] = {
        MockRedisServer{"master0"},
    };
    MockRedisServer slaves[2] = {
        MockRedisServer{"slave0"},
        MockRedisServer{"slave1"},
    };
    MockRedisServer sentinels[3] = {
        MockRedisServer{"sentinel0"},
        MockRedisServer{"sentinel1"},
        MockRedisServer{"sentinel2"},
    };
    std::shared_ptr<storages::redis::impl::ThreadPools>
        thread_pool = std::make_shared<storages::redis::impl::ThreadPools>(1, kRedisThreadCount);
};

}  // namespace

UTEST(Redis, CommandImmediatelyAfterConnected) {
    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();
    auto get_handler = server.RegisterHandlerWithConstReply("GET", std::string{"value"});

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};

    engine::SingleConsumerEvent connected;
    redis.signal_state_change.connect([&connected](storages::redis::RedisState state) {
        if (state == storages::redis::RedisState::kConnected) {
            connected.Send();
        }
    });

    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );
    ASSERT_TRUE(connected.WaitForEventFor(kSuccessTimeout));

    engine::SingleConsumerEvent got_reply;
    std::atomic<bool> reply_ok{false};
    auto command = storages::redis::impl::PrepareCommand(
        {"GET", "key"},
        [&](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            reply_ok = reply->IsOk() && reply->data.IsString() && reply->data.GetString() == "value";
            got_reply.Send();
        }
    );
    ASSERT_TRUE(redis.AsyncCommand(command));
    ASSERT_TRUE(got_reply.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(reply_ok.load());
    EXPECT_TRUE(get_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, NoPassword) {
    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, PendingUnsubscribeDisconnectsOnHeartbeatFailure) {
    RedisServerWithPendingUnsubscribe server;
    engine::SingleConsumerEvent subscribed;
    engine::SingleConsumerEvent subscription_finished;
    std::atomic<storages::redis::ReplyStatus> final_status{storages::redis::ReplyStatus::kOk};
    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );
    ASSERT_TRUE(server.WaitForFirstPingReply(kSuccessTimeout));

    auto subscribe = storages::redis::impl::PrepareCommand(
        {"SUBSCRIBE", kPendingUnsubscribeChannel},
        [&](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            if (reply->IsOk()) {
                subscribed.Send();
            } else {
                final_status = reply->status;
                subscription_finished.Send();
            }
        },
        storages::redis::CommandControl{kSuccessTimeout, kSuccessTimeout, 1}
    );
    ASSERT_TRUE(redis.AsyncCommand(subscribe));
    ASSERT_TRUE(subscribed.WaitForEventFor(kSuccessTimeout));

    ASSERT_TRUE(redis.AsyncCommand(storages::redis::impl::PrepareCommand(
        {"UNSUBSCRIBE", kPendingUnsubscribeChannel},
        storages::redis::impl::ReplyCallback{}
    )));
    ASSERT_TRUE(server.WaitForUnsubscribe());
    // The heartbeat SUBSCRIBE succeeded too: its command timeout cannot cause this disconnect.
    ASSERT_TRUE(server.WaitForHeartbeatUnsubscribe());
    ASSERT_TRUE(subscription_finished.WaitForEventFor(kSuccessTimeout));
    EXPECT_EQ(final_status.load(), storages::redis::ReplyStatus::kEndOfFileError);
    EXPECT_TRUE(server.WaitForDisconnect());
    EXPECT_FALSE(IsConnected(redis));
}

UTEST(Redis, CommandQueuedBeforeConnect) {
    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();
    auto set_handler = server.RegisterStatusReplyHandler("SET", "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};

    engine::SingleConsumerEvent callback_called;
    auto command = storages::redis::impl::PrepareCommand(
        {"SET", "key", "value"},
        [&callback_called](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            EXPECT_TRUE(reply->IsOk());
            callback_called.Send();
        }
    );
    ASSERT_TRUE(redis.AsyncCommand(command));

    redis.Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(callback_called.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(set_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, CommandsCounterHonorsStatisticsSettings) {
    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();
    auto ignored_handler = server.RegisterStatusReplyHandler("SET", {"ignored"}, "OK");
    auto failed_handler = server.RegisterErrorReplyHandler("SET", {"failed"}, "ERR test failure");
    auto counted_handler = server.RegisterStatusReplyHandler("SET", {"counted"}, "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    ASSERT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(redis); });
    EXPECT_EQ(redis.GetStatistics().commands_count.load(std::memory_order_relaxed), 0);

    engine::SingleConsumerEvent ignored_callback_called;
    storages::redis::CommandControl ignored_control;
    ignored_control.account_in_statistics = false;
    auto ignored_command = storages::redis::impl::PrepareCommand(
        {"SET", "ignored", "value"},
        [&ignored_callback_called](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            EXPECT_TRUE(reply->IsOk());
            ignored_callback_called.Send();
        },
        ignored_control
    );
    ASSERT_TRUE(redis.AsyncCommand(ignored_command));
    ASSERT_TRUE(ignored_callback_called.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(ignored_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_EQ(redis.GetStatistics().commands_count.load(std::memory_order_relaxed), 0);

    engine::SingleConsumerEvent failed_callback_called;
    auto failed_command = storages::redis::impl::PrepareCommand(
        {"SET", "failed", "value"},
        [&failed_callback_called](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            EXPECT_FALSE(reply->IsOk());
            failed_callback_called.Send();
        }
    );
    ASSERT_TRUE(redis.AsyncCommand(failed_command));
    ASSERT_TRUE(failed_callback_called.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(failed_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_EQ(redis.GetStatistics().commands_count.load(std::memory_order_relaxed), 0);

    engine::SingleConsumerEvent counted_callback_called;
    auto counted_command = storages::redis::impl::PrepareCommand(
        {"SET", "counted", "value"},
        [&counted_callback_called](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            EXPECT_TRUE(reply->IsOk());
            counted_callback_called.Send();
        }
    );
    ASSERT_TRUE(redis.AsyncCommand(counted_command));
    ASSERT_TRUE(counted_callback_called.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(counted_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_EQ(redis.GetStatistics().commands_count.load(std::memory_order_relaxed), 1);
}

UTEST_MT(Redis, ConcurrentAsyncCommandPreservesProducerOrder, 8) {
    constexpr std::size_t kProducers = 4;
    constexpr std::size_t kCommandsPerProducer = 100;
    constexpr std::size_t kCommandsTotal = kProducers * kCommandsPerProducer;

    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();
    auto set_handler = server.RegisterStatusReplyHandler("SET", "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(redis); });

    std::array<std::atomic<std::size_t>, kProducers> next_callback{};
    std::atomic<std::size_t> callbacks_count{0};
    engine::SingleConsumerEvent all_callbacks_called;
    std::vector<engine::TaskWithResult<void>> producers;
    producers.reserve(kProducers);

    for (std::size_t producer_idx = 0; producer_idx < kProducers; ++producer_idx) {
        producers.push_back(engine::AsyncNoTracing([&, producer_idx] {
            for (std::size_t command_idx = 0; command_idx < kCommandsPerProducer; ++command_idx) {
                storages::redis::CommandControl control;
                control.timeout_single = kSuccessTimeout;
                auto command = storages::redis::impl::PrepareCommand(
                    {"SET", std::to_string(producer_idx), std::to_string(command_idx)},
                    [&,
                     producer_idx,
                     command_idx](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
                        EXPECT_FALSE(engine::current_task::IsTaskProcessorThread());
                        EXPECT_TRUE(reply->IsOk());
                        EXPECT_EQ(next_callback[producer_idx].fetch_add(1), command_idx);
                        if (callbacks_count.fetch_add(1) + 1 == kCommandsTotal) {
                            all_callbacks_called.Send();
                        }
                    },
                    control
                );
                ASSERT_TRUE(redis.AsyncCommand(command));
            }
        }));
    }

    for (auto& producer : producers) {
        producer.Get();
    }

    ASSERT_TRUE(all_callbacks_called.WaitForEventFor(kSuccessTimeout));
    EXPECT_EQ(set_handler->GetReplyCount(), kCommandsTotal);
    EXPECT_EQ(callbacks_count.load(), kCommandsTotal);
    for (const auto& producer_callback : next_callback) {
        EXPECT_EQ(producer_callback.load(), kCommandsPerProducer);
    }
}

UTEST_MT(Redis, DisconnectRaceCompletesEveryAcceptedCommand, 8) {
    constexpr std::size_t kProducers = 4;
    constexpr std::size_t kEnqueuesBeforeDisconnect = 20;
    constexpr std::size_t kBurstCommandsPerProducer = 10000;
    constexpr std::chrono::milliseconds kThrottledEnqueueInterval{1};

    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();
    auto disconnect_reply = server.RegisterPausedReplyHandler(
        "GET",
        storages::redis::ReplyData::CreateError("READONLY You can't write against a read only slave")
    );
    auto set_handler = server.RegisterStatusReplyHandler("SET", "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(redis); });

    std::atomic<std::size_t> disconnect_callbacks{0};
    auto disconnect_command = storages::redis::impl::PrepareCommand(
        {"GET", "disconnect"},
        [&disconnect_callbacks](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr&) {
            ++disconnect_callbacks;
        }
    );
    ASSERT_TRUE(redis.AsyncCommand(disconnect_command));
    ASSERT_TRUE(disconnect_reply->WaitForRequest(kSuccessTimeout));

    std::atomic<std::size_t> enqueue_attempts{0};
    std::atomic<std::size_t> accepted_commands{0};
    std::atomic<std::size_t> rejected_commands{0};
    std::atomic<std::size_t> command_callbacks{0};
    std::vector<engine::TaskWithResult<void>> producers;
    producers.reserve(kProducers);

    for (std::size_t producer_idx = 0; producer_idx < kProducers; ++producer_idx) {
        producers.push_back(engine::AsyncNoTracing([&, producer_idx] {
            const auto deadline = engine::Deadline::FromDuration(kSuccessTimeout);
            for (std::size_t command_idx = 0; !deadline.IsReached(); ++command_idx) {
                storages::redis::CommandControl control;
                control.timeout_single = kSuccessTimeout;
                auto command = storages::redis::impl::PrepareCommand(
                    {"SET", std::to_string(producer_idx), std::to_string(command_idx)},
                    [&command_callbacks](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr&) {
                        ++command_callbacks;
                    },
                    control
                );
                ++enqueue_attempts;
                if (!redis.AsyncCommand(command)) {
                    ++rejected_commands;
                    return;
                }
                ++accepted_commands;
                if (command_idx < kBurstCommandsPerProducer) {
                    engine::Yield();
                } else {
                    // The ev thread processes the READONLY reply (and thus the disconnect) only after
                    // the commands enqueued before it, so the burst may outrun the disconnect.
                    // Keep producing, but slower, until the disconnect is processed.
                    engine::SleepFor(kThrottledEnqueueInterval);
                }
            }
            ADD_FAILURE() << "Redis did not stop accepting commands";
        }));
    }

    while (enqueue_attempts.load() < kEnqueuesBeforeDisconnect) {
        engine::Yield();
    }
    disconnect_reply->ReleaseReply();

    for (auto& producer : producers) {
        producer.Get();
    }

    PeriodicWait([&] { return command_callbacks.load() == accepted_commands.load(); });
    EXPECT_TRUE(redis.IsDestroying());
    EXPECT_EQ(rejected_commands.load(), kProducers);
    EXPECT_EQ(disconnect_callbacks.load(), 1);
    EXPECT_LE(set_handler->GetReplyCount(), accepted_commands.load());
}

UTEST(Redis, Auth) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_handler = server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, AuthWithUsername) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_handler = server.RegisterStatusReplyHandler("AUTH", {"username", "password"}, "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"username", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, AuthFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_error_handler = server.RegisterErrorReplyHandler("AUTH", "NO PASARAN");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(redis); });
}

UTEST(Redis, AuthTimeout) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto auth_error_handler = server.RegisterTimeoutHandler("AUTH", sleep_period);

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(sleep_period + kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(redis); });
}

UTEST(Redis, SentinelAuth) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SubscriptionDefaultsAreIndependentFromCommandDefaults) {
    using namespace storages::redis;
    MockRedisServer server{"standalone"};
    server.RegisterPingHandler();
    const auto subscribed = server.RegisterHandlerWithConstReply(
        "SUBSCRIBE",
        {"independent-defaults"},
        ReplyData::Array{ReplyData("subscribe"), ReplyData("independent-defaults"), ReplyData(1)}
    );
    server.RegisterHandlerWithConstReply(
        "UNSUBSCRIBE",
        {"independent-defaults"},
        ReplyData::Array{ReplyData("unsubscribe"), ReplyData("independent-defaults"), ReplyData(0)}
    );
    const auto thread_pool = std::make_shared<impl::ThreadPools>(1, 1);
    secdist::RedisSettings settings;
    settings.shards = {"standalone"};
    settings.sentinels.emplace_back(kLocalhost, server.GetPort());
    auto config = impl::SentinelStaticConfig{"test", {ShardingStrategy::kRedisStandalone}, {}, {}};
    config.connection_mode = impl::ConnectionMode::kCommandsAndSubscriptions;
    // A command-only server constraint must never leak into subscriptions.
    config.command_control.force_server_id = ServerId::Generate();
    config.subscription_command_control.emplace().allow_reads_from_master = true;
    const auto sentinel =
        impl::Sentinel::CreateSentinel(thread_pool, settings, "standalone", dynamic_config::GetDefaultSource(), config);
    const RedisWaitConnected wait{WaitConnectedMode::kMaster, true, kSuccessTimeout};
    sentinel->WaitConnectedOnce(wait);
    sentinel->WaitConnectedOnce(wait, impl::ConnectionKind::kSubscription);

    auto command_defaults = std::make_shared<CommandControl>();
    command_defaults->force_server_id = ServerId::Generate();
    sentinel->SetConfigDefaultCommandControl(command_defaults);
    CommandControl subscription_defaults;
    subscription_defaults.force_server_id = ServerId::Generate();
    sentinel->SetSubscriberDefaultCommandControl(subscription_defaults);
    subscription_defaults.force_server_id.reset();
    subscription_defaults.max_retries = 5;
    sentinel->SetSubscriberDefaultCommandControl(subscription_defaults);
    EXPECT_EQ(sentinel->GetCommandControl({}).force_server_id, command_defaults->force_server_id);
    EXPECT_NE(sentinel->GetCommandControl({}).max_retries, subscription_defaults.max_retries);

    auto token = sentinel->Subscribe("independent-defaults", [](const auto&, const auto&) {
        return impl::Sentinel::Outcome::kOk;
    });
    EXPECT_TRUE(subscribed->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, SubscriptionConnectionsDoNotMonitorReplication) {
    using namespace storages::redis;
    RedisServerWithReplicationMonitoring server;
    const auto auth = server.RegisterStatusReplyHandler("AUTH", {"new-password"}, "OK");
    impl::Statistics command_statistics;
    impl::Statistics subscription_statistics;
    const auto pool = std::make_shared<impl::ThreadPools>(1, 1);
    auto ev_thread = pool->GetSentinelThreadPool().NextThread();
    const ReplicationMonitoringSettings monitoring{true, true};
    const auto holder = ev_thread.RunInEvLoopSyncWithResult([&] {
        auto connection = impl::RedisConnectionHolder::Create(
            ev_thread,
            pool->GetRedisThreadPool(),
            kDbName,
            kLocalhost,
            server.GetPort(),
            Credentials{"", Password{""}},
            kDatabaseIndex,
            {},
            monitoring,
            {},
            command_statistics,
            impl::RedisConnectionHolder::makeSentinelNodeRedisCreationSettings()
        );
        connection->EnableSubscriptions(subscription_statistics);
        // Exercise dynamic updates as well as the initial connection settings.
        connection->SetReplicationMonitoringSettings(monitoring);
        return connection;
    });

    // Before SUBSCRIBE, an accidentally enabled monitor can still send INFO on
    // the subscription connection. Two INFO rounds give both timers a chance to run.
    ASSERT_TRUE(server.WaitForInfoRequests(2));
    EXPECT_EQ(server.GetInfoConnectionCount(), 1);
    EXPECT_TRUE(holder->GetConnection(impl::ConnectionKind::kCommand)->IsSyncing());
    EXPECT_FALSE(holder->GetConnection(impl::ConnectionKind::kSubscription)->IsSyncing());

    ev_thread.RunInEvLoopSync([&] { holder->UpdateCredentials(Credentials{"", Password{"new-password"}}); });
    const auto deadline = engine::Deadline::FromDuration(kSuccessTimeout);
    while (auth->GetReplyCount() < 2 && !deadline.IsReached()) {
        engine::SleepFor(kWaitPeriod);
    }
    ASSERT_EQ(auth->GetReplyCount(), 2);
    pool->SynchronizeEventLoops();
    server.ResetInfoRequests();

    ASSERT_TRUE(server.WaitForInfoRequests(2));
    EXPECT_EQ(server.GetInfoConnectionCount(), 1);
    EXPECT_TRUE(holder->GetConnection(impl::ConnectionKind::kCommand)->IsSyncing());
    EXPECT_FALSE(holder->GetConnection(impl::ConnectionKind::kSubscription)->IsSyncing());
}

UTEST(Redis, SubscriptionCommandsBypassCommandBuffering) {
    using namespace storages::redis;
    for (const auto mode : {impl::ConnectionMode::kCommandsAndSubscriptions, impl::ConnectionMode::kSubscriptions}) {
        const bool has_commands = impl::HasCommands(mode);
        SCOPED_TRACE(has_commands ? "shared" : "subscription-only");
        MockRedisServer server{kDbName};
        server.RegisterPingHandler();
        for (const auto* channel : {"buffering-initial", "buffering-dynamic", "buffering-reconnect"}) {
            RegisterSubscriptionHandlers(server, channel);
        }
        RegisterSubscriptionHandlers(server, std::string{impl::CmdWithArgs::kSubscriberPingChannelName});
        const auto gets = server.RegisterHandlerWithConstReply("GET", std::string{"value"});
        const auto auth = server.RegisterStatusReplyHandler("AUTH", {"new-password"}, "OK");
        impl::Statistics command_statistics;
        impl::Statistics subscription_statistics;
        const auto pool = std::make_shared<impl::ThreadPools>(1, 1);
        auto ev_thread = pool->GetSentinelThreadPool().NextThread();
        CommandsBufferingSettings buffering;
        buffering.buffering_enabled = true;
        buffering.commands_buffering_threshold = 2;
        buffering
            .watch_command_timer_interval = std::chrono::duration_cast<std::chrono::microseconds>(kSuccessTimeout * 2);
        const auto holder = ev_thread.RunInEvLoopSyncWithResult([&] {
            auto connection = impl::RedisConnectionHolder::Create(
                ev_thread,
                pool->GetRedisThreadPool(),
                kDbName,
                kLocalhost,
                server.GetPort(),
                Credentials{"", Password{""}},
                kDatabaseIndex,
                buffering,
                {},
                {},
                has_commands ? command_statistics : subscription_statistics,
                impl::RedisConnectionHolder::makeSentinelNodeRedisCreationSettings(),
                {},
                std::chrono::seconds{35},
                has_commands ? impl::ConnectionKind::kCommand : impl::ConnectionKind::kSubscription
            );
            if (has_commands) {
                connection->EnableSubscriptions(subscription_statistics);
            }
            return connection;
        });
        ASSERT_NO_FATAL_FAILURE(CheckSubscriptionsBypassBuffering(*holder, *gets, "buffering-initial", mode));
        buffering.watch_command_timer_interval *= 2;
        ev_thread.RunInEvLoopSync([&] { holder->SetCommandsBufferingSettings(buffering); });
        pool->SynchronizeEventLoops();
        ASSERT_NO_FATAL_FAILURE(CheckSubscriptionsBypassBuffering(*holder, *gets, "buffering-dynamic", mode));

        ev_thread.RunInEvLoopSync([&] { holder->UpdateCredentials(Credentials{"", Password{"new-password"}}); });
        const auto deadline = engine::Deadline::FromDuration(kSuccessTimeout);
        const size_t expected_auths = has_commands ? 2 : 1;
        while (auth->GetReplyCount() < expected_auths && !deadline.IsReached()) {
            engine::SleepFor(kWaitPeriod);
        }
        ASSERT_EQ(auth->GetReplyCount(), expected_auths);
        ASSERT_NO_FATAL_FAILURE(CheckSubscriptionsBypassBuffering(*holder, *gets, "buffering-reconnect", mode));
    }
}

UTEST(Redis, ConnectionModesCreateOnlyRequestedDataConnections) {
    using namespace storages::redis;
    for (const auto mode :
         {impl::ConnectionMode::kCommands,
          impl::ConnectionMode::kSubscriptions,
          impl::ConnectionMode::kCommandsAndSubscriptions})
    {
        SCOPED_TRACE(static_cast<int>(mode));
        MockRedisServer server{kDbName};
        server.RegisterPingHandler();
        const auto auth = server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK");
        const auto pool = std::make_shared<impl::ThreadPools>(1, 1);
        secdist::RedisSettings settings;
        settings.shards = {"standalone"};
        settings.sentinels.emplace_back(kLocalhost, server.GetPort());
        settings.password = Password{"password"};
        auto config = impl::SentinelStaticConfig{"test", {ShardingStrategy::kRedisStandalone}, {}, {}};
        config.connection_mode = mode;
        const auto sentinel =
            impl::Sentinel::CreateSentinel(pool, settings, kDbName, dynamic_config::GetDefaultSource(), config);
        const RedisWaitConnected wait{WaitConnectedMode::kMaster, true, kSuccessTimeout};
        const bool commands = mode != impl::ConnectionMode::kSubscriptions;
        const bool subscriptions = mode != impl::ConnectionMode::kCommands;
        if (commands) {
            sentinel->WaitConnectedOnce(wait, impl::ConnectionKind::kCommand);
        }
        if (subscriptions) {
            sentinel->WaitConnectedOnce(wait, impl::ConnectionKind::kSubscription);
        }
        const HealthCheckParams health{WaitConnectedMode::kMaster, 0, 0};
        EXPECT_EQ(sentinel->IsReady(health, impl::ConnectionKind::kCommand), commands);
        EXPECT_EQ(sentinel->IsReady(health, impl::ConnectionKind::kSubscription), subscriptions);
        EXPECT_EQ(auth->GetReplyCount(), static_cast<size_t>(commands) + static_cast<size_t>(subscriptions));
    }
}

UTEST(Redis, SubscriptionOnlySupportsNearestAndForcedServerRouting) {
    using namespace storages::redis;
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    const auto register_subscriptions = [](auto& server) {
        RegisterSubscriptionHandlers(server, "nearest-subscriber");
        RegisterSubscriptionHandlers(server, "forced-subscriber");
        RegisterSubscriptionHandlers(server, std::string{impl::CmdWithArgs::kSubscriberPingChannelName});
    };
    for (auto& server : mock.masters) {
        register_subscriptions(server);
    }
    for (auto& server : mock.slaves) {
        register_subscriptions(server);
    }
    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    for (const auto& server : mock.sentinels) {
        settings.sentinels.emplace_back(kLocalhost, server.GetPort());
    }
    impl::SubscribeRedisGroup group;
    group.db = "subscriber";
    const auto sentinel = impl::Sentinel::CreateSentinel(
        mock.thread_pool,
        settings,
        kDbName,
        dynamic_config::GetDefaultSource(),
        impl::MakeSentinelStaticConfig(group)
    );
    sentinel->WaitConnectedDebug(false, impl::ConnectionKind::kSubscription);
    const HealthCheckParams health{WaitConnectedMode::kMasterAndSlave, 0, 0};
    EXPECT_TRUE(sentinel->IsReady(health, impl::ConnectionKind::kSubscription));
    EXPECT_FALSE(sentinel->IsReady(health, impl::ConnectionKind::kCommand));

    CommandControl cc;
    cc.strategy = CommandControl::Strategy::kNearestServerPing;
    cc.best_dc_count = 1;
    ServerId selected_server;
    ASSERT_NO_FATAL_FAILURE(CheckSubscriptionRouting(*sentinel, "nearest-subscriber", cc, selected_server));

    cc.force_server_id = selected_server;
    ASSERT_NO_FATAL_FAILURE(CheckSubscriptionRouting(*sentinel, "forced-subscriber", cc, selected_server));
}

UTEST(Redis, SubscriptionConnectionsUpdateCredentials) {
    MockRedisServer server{"standalone"};
    server.RegisterPingHandler();
    const auto auth = server.RegisterStatusReplyHandler("AUTH", {"new-password"}, "OK");
    const auto thread_pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);

    secdist::RedisSettings settings;
    settings.shards = {"standalone"};
    settings.sentinels.emplace_back(kLocalhost, server.GetPort());
    auto config = storages::redis::impl::SentinelStaticConfig{
        "test_client_name",
        {storages::redis::ShardingStrategy::kRedisStandalone},
        {},
        {}
    };
    config.connection_mode = storages::redis::impl::ConnectionMode::kCommandsAndSubscriptions;
    auto sentinel = storages::redis::impl::Sentinel::CreateSentinel(
        thread_pool,
        settings,
        "standalone",
        dynamic_config::GetDefaultSource(),
        config
    );
    const storages::redis::RedisWaitConnected wait{storages::redis::WaitConnectedMode::kMaster, true, kSuccessTimeout};
    sentinel->WaitConnectedOnce(wait);
    sentinel->WaitConnectedOnce(wait, storages::redis::impl::ConnectionKind::kSubscription);

    settings.password = storages::redis::Password{"new-password"};
    sentinel->UpdateSettings(settings);
    const auto deadline = engine::Deadline::FromDuration(kSuccessTimeout);
    while (auth->GetReplyCount() < 2 && !deadline.IsReached()) {
        engine::SleepFor(std::chrono::milliseconds{10});
    }
    EXPECT_EQ(auth->GetReplyCount(), 2);
}

UTEST(Redis, ConnectionHolderPreservesRoleStateAcrossReconnects) {
    using namespace storages::redis;
    for (const auto kind : {impl::ConnectionKind::kCommand, impl::ConnectionKind::kSubscription}) {
        SCOPED_TRACE(kind == impl::ConnectionKind::kCommand ? "command" : "subscription");
        RedisServerForConnectionLifecycle server;
        impl::Statistics command_statistics;
        impl::Statistics subscription_statistics;
        const auto pool = std::make_shared<impl::ThreadPools>(1, 1);
        auto ev_thread = pool->GetSentinelThreadPool().NextThread();
        auto holder = ev_thread.RunInEvLoopSyncWithResult([&] {
            auto connection = impl::RedisConnectionHolder::Create(
                ev_thread,
                pool->GetRedisThreadPool(),
                kDbName,
                kLocalhost,
                server.GetPort(),
                {},
                kDatabaseIndex,
                {},
                {},
                {},
                command_statistics,
                impl::RedisConnectionHolder::makeSentinelNodeRedisCreationSettings(),
                {},
                std::chrono::seconds::zero()
            );
            connection->EnableSubscriptions(subscription_statistics);
            return connection;
        });
        const auto wait_connected = [&](const std::shared_ptr<impl::Redis>& redis) {
            ASSERT_TRUE(redis);
            const auto connected = std::make_shared<engine::SingleConsumerEvent>();
            const boost::signals2::scoped_connection observer{
                redis->signal_state_change.connect([connected](impl::Redis::State state) {
                    if (state == impl::Redis::State::kConnected) {
                        connected->Send();
                    }
                })
            };
            if (redis->GetState() != impl::Redis::State::kConnected) {
                ASSERT_TRUE(connected->WaitForEventFor(kSuccessTimeout));
            }
            pool->SynchronizeEventLoops();
        };
        const auto pause_disconnect =
            [&](const std::shared_ptr<impl::Redis>& redis, MockRedisServer::PausedReplyPtr& reply) {
                reply = server.RegisterPausedReplyHandler(
                    "GET",
                    ReplyData::CreateError("READONLY You can't write against a read only slave")
                );
                CommandControl control;
                control.timeout_single = kSuccessTimeout * 4;
                ASSERT_TRUE(redis->AsyncCommand(
                    impl::PrepareCommand({"GET", "disconnect"}, [](const auto&, const auto&) {}, control)
                ));
                ASSERT_TRUE(reply->WaitForRequest(kSuccessTimeout));
            };
        const auto watch_disconnect =
            [](const std::shared_ptr<impl::Redis>& redis,
               const std::shared_ptr<engine::SingleConsumerEvent>& disconnected) {
                return boost::signals2::scoped_connection{
                    redis->signal_state_change.connect([disconnected](impl::Redis::State state) {
                        if (state == impl::Redis::State::kDisconnected || state == impl::Redis::State::kDisconnectError)
                        {
                            disconnected->Send();
                        }
                    })
                };
            };
        for (const auto role : {impl::ConnectionKind::kCommand, impl::ConnectionKind::kSubscription}) {
            ASSERT_NO_FATAL_FAILURE(wait_connected(holder->GetConnection(role)));
        }
        {
            const auto previous = holder->GetConnection(kind);
            const auto sibling_kind =
                kind == impl::ConnectionKind::kCommand
                    ? impl::ConnectionKind::kSubscription
                    : impl::ConnectionKind::kCommand;
            const auto sibling = holder->GetConnection(sibling_kind);
            MockRedisServer::PausedReplyPtr reply;
            ASSERT_NO_FATAL_FAILURE(pause_disconnect(previous, reply));
            const auto reconnected = std::make_shared<engine::SingleConsumerEvent>();
            const boost::signals2::scoped_connection observer{
                holder->signal_state_change
                    .connect([weak_holder = std::weak_ptr{holder}, kind, previous, reconnected](impl::Redis::State state
                             ) {
                        const auto holder = weak_holder.lock();
                        if (!holder) {
                            return;
                        }
                        const auto current = holder->GetConnection(kind);
                        if (state == impl::Redis::State::kConnected && current != previous &&
                            current->GetState() == impl::Redis::State::kConnected)
                        {
                            reconnected->Send();
                        }
                    })
            };
            reply->ReleaseReply();
            ASSERT_TRUE(reconnected->WaitForEventFor(kSuccessTimeout));
            pool->SynchronizeEventLoops();
            EXPECT_NE(holder->GetConnection(kind), previous);
            EXPECT_EQ(holder->GetConnection(sibling_kind), sibling);
            EXPECT_TRUE(holder->IsReady(kind));
            EXPECT_TRUE(holder->IsReady(sibling_kind));
        }
        {
            const auto previous = holder->GetConnection(kind);
            MockRedisServer::PausedReplyPtr reply;
            ASSERT_NO_FATAL_FAILURE(pause_disconnect(previous, reply));
            const auto disconnected = std::make_shared<engine::SingleConsumerEvent>();
            const auto disconnect_observer = watch_disconnect(previous, disconnected);
            const auto forwarded_disconnects = std::make_shared<std::atomic<std::size_t>>(0);
            const boost::signals2::scoped_connection observer{
                holder->signal_state_change.connect([forwarded_disconnects](impl::Redis::State state) {
                    if (state == impl::Redis::State::kDisconnecting || state == impl::Redis::State::kDisconnected ||
                        state == impl::Redis::State::kDisconnectError)
                    {
                        ++*forwarded_disconnects;
                    }
                })
            };
            ev_thread.RunInEvLoopSync([&] { holder->UpdateCredentials(Credentials{"", Password{"new-password"}}); });
            for (const auto role : {impl::ConnectionKind::kCommand, impl::ConnectionKind::kSubscription}) {
                ASSERT_NO_FATAL_FAILURE(wait_connected(holder->GetConnection(role)));
            }
            ASSERT_NE(holder->GetConnection(kind), previous);
            ASSERT_EQ(forwarded_disconnects->load(), 0);
            reply->ReleaseReply();
            ASSERT_TRUE(disconnected->WaitForEventFor(kSuccessTimeout));
            pool->SynchronizeEventLoops();
            EXPECT_EQ(forwarded_disconnects->load(), 0);
            for (const auto role : {impl::ConnectionKind::kCommand, impl::ConnectionKind::kSubscription}) {
                EXPECT_EQ(holder->GetState(role), impl::Redis::State::kConnected);
                EXPECT_TRUE(holder->IsReady(role));
            }
        }
        {
            const auto redis = holder->GetConnection(kind);
            MockRedisServer::PausedReplyPtr reply;
            ASSERT_NO_FATAL_FAILURE(pause_disconnect(redis, reply));
            const auto disconnected = std::make_shared<engine::SingleConsumerEvent>();
            const auto observer = watch_disconnect(redis, disconnected);
            const std::weak_ptr<impl::RedisConnectionHolder> weak_holder{holder};
            holder.reset();
            EXPECT_TRUE(weak_holder.expired());
            reply->ReleaseReply();
            ASSERT_TRUE(disconnected->WaitForEventFor(kSuccessTimeout));
            pool->SynchronizeEventLoops();
        }
    }
}

UTEST(Redis, SentinelAuthWithUsername) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_username = "sentinel_username";
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers
            .push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_username", "sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelAuthWithUsernameAndPassword) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.username = "username";
    settings.password = storages::redis::Password("password");
    settings.sentinel_username = "sentinel_username";
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers
            .push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_username", "sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> data_auth_handlers;
    data_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        data_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"username", "password"}, "FAIL"));
    }
    for (auto& server : slaves) {
        data_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"username", "password"}, "FAIL"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : data_auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelNoAuthButPassword) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.password = storages::redis::Password("password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        no_auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }
    for (auto& server : slaves) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelNoAuth) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels) + std::size(masters) + std::size(slaves));
    mock.ForEachServer([&](auto& server) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    });

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelAuthSubscribe) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, SentinelAuthSubscribeWithUsername) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_username = "sentinel_username";
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers
            .push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_username", "sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, SentinelNoAuthSubscribeButPassword) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.password = storages::redis::Password("password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        no_auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }
    for (auto& server : slaves) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, SentinelNoAuthSubscribe) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels) + std::size(masters) + std::size(slaves));
    mock.ForEachServer([&](auto& server) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    });

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, Select) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto select_handler = server.RegisterStatusReplyHandler("SELECT", "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, SelectFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto select_error_handler = server.RegisterErrorReplyHandler("SELECT", "NO PASARAN");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(redis); });
}

UTEST(Redis, SelectTimeout) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto select_error_handler = server.RegisterTimeoutHandler("SELECT", sleep_period);

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(sleep_period + kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(redis); });
}

UTEST_MT(Redis, SlaveREADONLY, 2) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto readonly_handler = server.RegisterStatusReplyHandler("READONLY", "OK");

    storages::redis::impl::ThreadPools pool{1, 1};
    storages::redis::RedisCreationSettings redis_settings;
    redis_settings.send_readonly = true;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(redis); });
}

UTEST(Redis, SlaveREADONLYFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto readonly_handler = server.RegisterErrorReplyHandler("READONLY", "FAIL");

    storages::redis::impl::ThreadPools pool{1, 1};
    storages::redis::RedisCreationSettings redis_settings;
    redis_settings.send_readonly = true;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return !IsConnected(redis); });
}

UTEST(Redis, PingFail) {
    MockRedisServer server{"redis_db"};
    auto ping_error_handler = server.RegisterErrorReplyHandler("PING", "PONG");

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );

    EXPECT_TRUE(ping_error_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return !IsConnected(redis); });
}

UTEST(Redis, TimedOutCommandReleasesCallback) {
    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();
    auto get_reply = server.RegisterPausedReplyHandler("GET", storages::redis::ReplyData::CreateStatus("OK"));
    auto set_handler = server.RegisterStatusReplyHandler("SET", "OK");

    std::atomic<bool> timeout_seen{false};
    std::atomic<bool> follow_up_ok{false};
    std::atomic<std::size_t> callback_count{0};
    engine::SingleConsumerEvent timeout_event;
    engine::SingleConsumerEvent lifetime_destroyed;
    engine::SingleConsumerEvent follow_up_received;

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(redis); });

    struct LifetimeNotifier {
        explicit LifetimeNotifier(engine::SingleConsumerEvent& destroyed)
            : destroyed(destroyed)
        {}

        ~LifetimeNotifier() { destroyed.Send(); }

        engine::SingleConsumerEvent& destroyed;
    };

    auto lifetime = std::make_shared<LifetimeNotifier>(lifetime_destroyed);
    const std::weak_ptr<LifetimeNotifier> weak_lifetime = lifetime;
    storages::redis::CommandControl command_control;
    command_control.timeout_single = std::chrono::milliseconds{100};
    auto command = storages::redis::impl::PrepareCommand(
        {"GET", "key"},
        [lifetime = std::move(lifetime),
         &timeout_seen,
         &callback_count,
         &timeout_event](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            static_cast<void>(lifetime);
            ++callback_count;
            timeout_seen = reply->status == storages::redis::ReplyStatus::kTimeoutError;
            timeout_event.Send();
        },
        command_control
    );
    ASSERT_TRUE(redis.AsyncCommand(command));
    command.reset();

    ASSERT_TRUE(get_reply->WaitForRequest(kSuccessTimeout));
    EXPECT_TRUE(timeout_event.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(lifetime_destroyed.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(timeout_seen.load());
    EXPECT_TRUE(weak_lifetime.expired());

    get_reply->ReleaseReply();
    ASSERT_TRUE(get_reply->WaitForReplySent(kSuccessTimeout));

    storages::redis::CommandControl follow_up_control;
    follow_up_control.timeout_single = kSuccessTimeout;
    auto follow_up = storages::redis::impl::PrepareCommand(
        {"SET", "key", "ready"},
        [&follow_up_ok,
         &follow_up_received](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            follow_up_ok = reply->status == storages::redis::ReplyStatus::kOk;
            follow_up_received.Send();
        },
        follow_up_control
    );
    ASSERT_TRUE(redis.AsyncCommand(follow_up));
    ASSERT_TRUE(follow_up_received.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(follow_up_ok.load());
    EXPECT_TRUE(set_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_EQ(callback_count.load(), 1);
}

class RedisDisconnectingReplies : public ::testing::TestWithParam<const char*> {};

INSTANTIATE_UTEST_SUITE_P(
    /**/,
    RedisDisconnectingReplies,
    ::testing::Values(
        "MASTERDOWN Link with MASTER is down and "
        "slave-serve-stale-data is set to 'no'.",
        "LOADING Redis is loading the dataset in memory",
        "READONLY You can't write against a read only slave"
    )
);

UTEST_P(RedisDisconnectingReplies, X) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto get_handler = server.RegisterErrorReplyHandler("GET", GetParam());

    storages::redis::impl::ThreadPools pool{1, 1};
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    storages::redis::impl::Redis redis{pool.GetRedisThreadPool(), redis_settings, kDbName, stats};
    redis.Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(redis); });

    auto cmd = storages::redis::impl::PrepareCommand(
        {"GET", "123"},
        [](const storages::redis::impl::CommandPtr&, storages::redis::ReplyPtr) {}
    );
    redis.AsyncCommand(cmd);

    EXPECT_TRUE(get_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return !IsConnected(redis); });
}

USERVER_NAMESPACE_END

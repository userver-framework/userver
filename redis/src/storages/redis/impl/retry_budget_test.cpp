#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/utils/retry_budget.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/mock_server_test.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/server_common_sentinel_test.hpp>
#include <storages/redis/impl/thread_pools.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kGetCommand = "GET";
constexpr auto kEvalShaCommand = "EVALSHA";
constexpr auto kHealthyKey = "healthy";
constexpr auto kTransientKey = "transient";
constexpr auto kPermanentKey = "permanent";
constexpr auto kRedirectKey = "redirect";
constexpr auto kRetryableError = "TRYAGAIN temporary failure";
constexpr auto kConnectionName = "retry-budget";
constexpr auto kOkReply = "OK";

storages::redis::CommandControl MakeRetryControl() {
    storages::redis::CommandControl control;
    control.timeout_single = kSuccessTimeout;
    control.timeout_all = kSuccessTimeout;
    control.max_retries = 4;
    return control;
}

struct ReplyState {
    storages::redis::ReplyPtr reply;
    engine::SingleConsumerEvent received;
    std::atomic<int> calls{0};
};

auto MakeReplyCallback(const std::shared_ptr<ReplyState>& state) {
    return [state](const storages::redis::impl::CommandPtr&, storages::redis::ReplyPtr reply) {
        if (++state->calls == 1) {
            state->reply = std::move(reply);
            state->received.Send();
        }
    };
}

storages::redis::ReplyPtr SendToConnection(
    storages::redis::impl::Redis& redis,
    const std::string& key,
    int counter = 0
) {
    const auto state = std::make_shared<ReplyState>();
    const auto command = storages::redis::impl::PrepareCommand(
        {kGetCommand, key},
        MakeReplyCallback(state),
        MakeRetryControl(),
        counter
    );
    const auto accepted = redis.AsyncCommand(command);
    EXPECT_TRUE(accepted);
    if (!accepted || !state->received.WaitForEventFor(kSuccessTimeout)) {
        ADD_FAILURE() << "Redis command did not complete";
        return {};
    }
    return state->reply;
}

class RedisRetryPolicy : public ::testing::TestWithParam<const char*> {};

INSTANTIATE_UTEST_SUITE_P(
    ServerErrors,
    RedisRetryPolicy,
    ::testing::Values(
        "OOM command not allowed when used memory > 'maxmemory'.",
        "ERR invalid command arguments",
        "WRONGTYPE Operation against a key holding the wrong kind of value",
        "NOSCRIPT No matching script. Please use EVAL.",
        "UNKNOWN unrecognized server error"
    )
);

UTEST_P(RedisRetryPolicy, NonRetryableServerErrorsAreReturnedOnce) {
    ClusterTest cluster{1};
    auto& sentinel = cluster.SentinelClient();
    const std::string error = GetParam();
    const auto handler = cluster.Master().RegisterErrorReplyHandler(kEvalShaCommand, error);

    const auto reply = sentinel.MakeRequest({kEvalShaCommand, "hash", "1", "key"}, 0, true, MakeRetryControl()).Get();

    ASSERT_TRUE(reply->data.IsError());
    EXPECT_EQ(reply->data.GetError(), error);
    EXPECT_EQ(reply->status, storages::redis::ReplyStatus::kOtherError);
    EXPECT_EQ(handler->GetReplyCount(), 1);
}

UTEST(RedisRetryBudget, AccountsFailuresAndAllowsRecovery) {
    MockRedisServer server{kConnectionName};
    server.RegisterPingHandler();
    server.RegisterStatusReplyHandler(kGetCommand, {kHealthyKey}, kOkReply);
    server.RegisterErrorReplyHandler(kGetCommand, {kTransientKey}, kRetryableError);
    server.RegisterErrorReplyHandler(kGetCommand, {kPermanentKey}, "OOM memory limit reached");
    server.RegisterErrorReplyHandler(kGetCommand, {kRedirectKey}, "MOVED 1 127.0.0.1:1234");

    const auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::impl::Statistics statistics;
    storages::redis::impl::Redis redis{pool->GetRedisThreadPool(), {}, kConnectionName, statistics};
    utils::RetryBudgetSettings settings{.max_tokens = 3, .token_ratio = 0.001, .enabled = true};
    const auto apply_settings = [&pool, &redis, &settings] {
        redis.SetRetryBudgetSettings(settings);
        pool->SynchronizeEventLoops();
    };
    apply_settings();
    redis.Connect({kLocalhost}, server.GetPort(), {}, 0);
    ASSERT_TRUE(server.WaitForFirstPingReply(kSuccessTimeout));
    ASSERT_TRUE(SendToConnection(redis, kHealthyKey));
    ASSERT_TRUE(redis.CanRetry());

    ASSERT_TRUE(SendToConnection(redis, kTransientKey));
    ASSERT_TRUE(SendToConnection(redis, kTransientKey));
    ASSERT_FALSE(redis.CanRetry());
    const auto retry = storages::redis::impl::PrepareCommand(
        {kGetCommand, kHealthyKey},
        [](const auto&, const auto&) {},
        MakeRetryControl(),
        1
    );
    EXPECT_FALSE(redis.AsyncCommand(retry));

    ASSERT_TRUE(SendToConnection(redis, kPermanentKey));
    ASSERT_TRUE(SendToConnection(redis, kRedirectKey));
    EXPECT_FALSE(redis.CanRetry());

    settings.enabled = false;
    apply_settings();
    ASSERT_TRUE(SendToConnection(redis, kHealthyKey));
    EXPECT_TRUE(redis.CanRetry());
    ASSERT_TRUE(SendToConnection(redis, kHealthyKey, 1));

    settings.enabled = true;
    apply_settings();
    ASSERT_TRUE(SendToConnection(redis, kPermanentKey));
    EXPECT_FALSE(redis.CanRetry());
    settings.token_ratio = 1;
    apply_settings();
    ASSERT_TRUE(SendToConnection(redis, kHealthyKey));
    EXPECT_TRUE(redis.CanRetry());
    ASSERT_TRUE(SendToConnection(redis, kHealthyKey, 1));
}

enum class ClusterOperation { kWrite, kForcedRead };

class RedisRetryBudgetCluster : public ::testing::TestWithParam<ClusterOperation> {};

INSTANTIATE_UTEST_SUITE_P(
    Operations,
    RedisRetryBudgetCluster,
    ::testing::Values(ClusterOperation::kWrite, ClusterOperation::kForcedRead)
);

UTEST_P(RedisRetryBudgetCluster, ExhaustedBudgetStopsRetries) {
    ClusterTest cluster{1};
    auto& sentinel = cluster.SentinelClient();
    cluster.SetRetryBudgetSettings({.max_tokens = 1, .token_ratio = 0.001, .enabled = true});
    const auto seed = sentinel.MakeRequest({kGetCommand, "seed"}, 0, true, MakeRetryControl()).Get();
    ASSERT_TRUE(seed->IsOk());

    auto control = MakeRetryControl();
    control.timeout_all = std::chrono::milliseconds{300};
    const auto is_write = GetParam() == ClusterOperation::kWrite;
    const auto command_name = is_write ? kEvalShaCommand : kGetCommand;
    if (!is_write) {
        control.force_server_id = seed->server_id;
    }
    const auto handler = cluster.Master().RegisterErrorReplyHandler(command_name, kRetryableError);
    const auto reply = sentinel.MakeRequest({command_name, "retry"}, 0, is_write, control).Get();
    EXPECT_EQ(reply->status, storages::redis::ReplyStatus::kTimeoutError);
    cluster.ResetSentinelClient();
    EXPECT_EQ(handler->GetReplyCount(), 1);
}

class RedisRetryBudgetRedirect : public ::testing::TestWithParam<std::tuple<const char*, bool>> {};

INSTANTIATE_UTEST_SUITE_P(
    Redirects,
    RedisRetryBudgetRedirect,
    ::testing::Combine(::testing::Values("MOVED", "ASK"), ::testing::Bool())
);

UTEST_P(RedisRetryBudgetRedirect, CompletesOnceAndChecksTargetBudget) {
    const auto [redirect_code, exhaust_budget] = GetParam();
    ClusterTest cluster{2};
    auto& sentinel = cluster.SentinelClient();
    cluster.SetRetryBudgetSettings({.max_tokens = 1, .token_ratio = 0.001, .enabled = true});
    const auto seed = sentinel.MakeRequest({kGetCommand, "seed"}, 1, true, MakeRetryControl()).Get();
    ASSERT_TRUE(seed->IsOk());
    if (exhaust_budget) {
        cluster.Master(1).RegisterErrorReplyHandler(kGetCommand, {"exhaust"}, kRetryableError);
        auto control = MakeRetryControl();
        control.max_retries = 1;
        const auto failure = sentinel.MakeRequest({kGetCommand, "exhaust"}, 1, true, control).Get();
        ASSERT_EQ(failure->status, storages::redis::ReplyStatus::kOtherError);
    }
    const auto redirect_error = fmt::format("{} 1 127.0.0.1:{}", redirect_code, cluster.Master(1).GetPort());
    cluster.Master().RegisterErrorReplyHandler(kEvalShaCommand, redirect_error);
    const auto target_handler = cluster.Master(1).RegisterStatusReplyHandler(kEvalShaCommand, kOkReply);
    cluster.Master(1).RegisterStatusReplyHandler("ASKING", kOkReply);

    const auto state = std::make_shared<ReplyState>();
    auto control = MakeRetryControl();
    control.timeout_single = kSuccessTimeout;
    control.timeout_all = kSuccessTimeout;
    const auto command =
        storages::redis::impl::PrepareCommand({kEvalShaCommand, "hash", "1", "key"}, MakeReplyCallback(state), control);
    sentinel.AsyncCommand(command, true, 0);
    ASSERT_TRUE(state->received.WaitForEventFor(kSuccessTimeout));
    cluster.ResetSentinelClient();

    EXPECT_EQ(state->calls.load(), 1);
    ASSERT_TRUE(state->reply);
    if (exhaust_budget) {
        EXPECT_EQ(target_handler->GetReplyCount(), 0);
        ASSERT_TRUE(state->reply->data.IsError()
        ) << state->reply->GetStatusString()
          << ": " << state->reply->data.ToDebugString();
        EXPECT_EQ(state->reply->data.GetError(), redirect_error);
    } else {
        EXPECT_TRUE(state->reply->IsOk());
        EXPECT_EQ(target_handler->GetReplyCount(), 1);
    }
}

}  // namespace

USERVER_NAMESPACE_END

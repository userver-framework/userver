#include <userver/utest/utest.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/mock_client_base.hpp>
#include <userver/storages/redis/mock_request.hpp>
#include <userver/storages/redis/mock_transaction.hpp>
#include <userver/storages/redis/mock_transaction_impl_base.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/transaction.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/mock_server_test.hpp>
#include <storages/redis/impl/secdist_redis.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/thread_pools.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace {

constexpr auto kShardName = "transaction-shard";
constexpr auto kLocalhost = "127.0.0.1";
constexpr auto kKey = "key";
constexpr auto kValue = "value";

class OptimisticTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        master_.RegisterPingHandler();
        replica_.RegisterPingHandler();
        sentinel_server_.RegisterPingHandler();
        sentinel_server_.RegisterSentinelMastersHandler({{kShardName, kLocalhost, master_.GetPort()}});
        sentinel_server_.RegisterSentinelSlavesHandler(kShardName, {{kShardName, kLocalhost, replica_.GetPort()}});
        for (auto* server : {&master_, &replica_}) {
            server->RegisterStatusReplyHandler("MULTI", "OK");
            server->RegisterStatusReplyHandler("GET", "QUEUED");
        }
        secdist::RedisSettings settings;
        settings.shards = {kShardName};
        settings.sentinels.emplace_back(kLocalhost, sentinel_server_.GetPort());
        auto sentinel = impl::Sentinel::CreateSentinel(
            pools_,
            settings,
            kShardName,
            dynamic_config::GetDefaultSource(),
            impl::SentinelStaticConfig{"transaction-test", {ShardingStrategy::kKeyShardTaximeterCrc32}, {}, {}}
        );
        sentinel->WaitConnectedDebug(false);
        client_ = std::make_shared<ClientImpl>(std::move(sentinel));
    }

    MockRedisServer& GetMaster() { return master_; }

    MockRedisServer& GetReplica() { return replica_; }

    const ClientPtr& GetClient() const { return client_; }

private:
    MockRedisServer master_{"master"};
    MockRedisServer replica_{"replica"};
    MockRedisServer sentinel_server_{"sentinel"};
    std::shared_ptr<impl::ThreadPools> pools_ = std::make_shared<impl::ThreadPools>(1, 1);
    ClientPtr client_;
};

class ConditionalTransactionMock final : public MockTransactionImplBase {
public:
    explicit ConditionalTransactionMock(ExecResult result)
        : result_(result)
    {}

    RequestGet Get(std::string key) override {
        EXPECT_EQ(key, kKey);
        return CreateMockRequest<RequestGet>(std::string{kValue});
    }

    ExecResult Exec(ExecOptions options) override {
        EXPECT_EQ(options.size(), 1);
        EXPECT_EQ(options.front().GetKey(), kKey);
        return result_;
    }

private:
    const ExecResult result_;
};

}  // namespace

UTEST_F(OptimisticTransactionTest, DocumentationExample) {
    GetMaster().RegisterStatusReplyHandler("SET", "QUEUED");
    auto exec = GetMaster().RegisterHandlerWithConstReply(
        "EXEC",
        {"IFEQ", "{account}version", "1", "XX", "{account}balance"},
        ReplyData::Array{ReplyData::CreateStatus("OK"), ReplyData::CreateStatus("OK")}
    );
    auto client = GetClient();
    const CommandControl command_control{};

    /// [optimistic transaction sample]
    using storages::redis::ExecCondition;

    const std::string expected_version = "1";
    const std::string next_version = "2";
    const std::string new_balance = "100";
    auto transaction = client->Multi();
    auto set = transaction->Set("{account}balance", new_balance);
    auto version = transaction->Set("{account}version", next_version);
    transaction
        ->Exec(
            command_control,
            {
                ExecCondition::IfEq("{account}version", expected_version),
                ExecCondition::Xx("{account}balance"),
            }
        )
        .Get();
    set.Get();
    version.Get();
    /// [optimistic transaction sample]

    EXPECT_EQ(exec->GetReplyCount(), 1);
}

UTEST_F(OptimisticTransactionTest, ReadOnlyConditionalExecUsesMaster) {
    auto master_exec = GetMaster().RegisterHandlerWithConstReply(
        "EXEC",
        {"IFEQ", kKey, kValue, "NX", "missing"},
        ReplyData::Array{std::string{kValue}}
    );
    auto replica_exec = GetReplica().RegisterErrorReplyHandler("EXEC", "ERR unexpected replica request");
    auto transaction = GetClient()->Multi();
    auto get = transaction->Get(kKey);
    transaction->Exec({}, ExecOptions{ExecCondition::IfEq(kKey, kValue), ExecCondition::Nx("missing")}).Get();
    EXPECT_EQ(get.Get(), kValue);
    EXPECT_EQ(master_exec->GetReplyCount(), 1);
    EXPECT_EQ(replica_exec->GetReplyCount(), 0);
}

UTEST_F(OptimisticTransactionTest, EmptyOptionsKeepOrdinaryExec) {
    GetMaster().RegisterHandlerWithConstReply("EXEC", ReplyData::Array{std::string{kValue}});
    GetReplica().RegisterHandlerWithConstReply("EXEC", ReplyData::Array{std::string{kValue}});
    auto transaction = GetClient()->Multi();
    auto get = transaction->Get(kKey);
    UEXPECT_NO_THROW(transaction->Exec({}, ExecOptions{}).Get());
    EXPECT_EQ(get.Get(), kValue);
}

UTEST_F(OptimisticTransactionTest, ConditionsDetermineShardWithoutSubcommands) {
    GetMaster().RegisterHandlerWithConstReply("EXEC", {"NX", kKey}, ReplyData::Array{});
    auto transaction = GetClient()->Multi();
    UEXPECT_NO_THROW(transaction->Exec({}, ExecCondition::Nx(kKey)).Get());
}

UTEST_F(OptimisticTransactionTest, NilFailsExecAndEverySubrequest) {
    auto exec = GetMaster().RegisterNilReplyHandler("EXEC", {"XX", kKey});
    auto transaction = GetClient()->Multi();
    auto first = transaction->Get(kKey);
    auto second = transaction->Get(kKey);
    auto request = transaction->Exec({}, ExecOptions{ExecCondition::Xx(kKey)});
    UEXPECT_THROW(request.Get(), TransactionAbortedException);
    UEXPECT_THROW(first.Get(), TransactionAbortedException);
    UEXPECT_THROW(second.Get(), TransactionAbortedException);
    EXPECT_EQ(exec->GetReplyCount(), 1);
}

UTEST_F(OptimisticTransactionTest, ServerErrorRemainsRequestFailure) {
    GetMaster().RegisterErrorReplyHandler("EXEC", "ERR wrong number of arguments for EXEC");
    auto transaction = GetClient()->Multi();
    auto get = transaction->Get(kKey);
    UEXPECT_THROW(transaction->Exec({}, ExecOptions{ExecCondition::Xx(kKey)}).Get(), RequestFailedException);
}

UTEST(OptimisticTransactionMock, SuccessAndAbort) {
    using ExecResult = MockTransactionImplBase::ExecResult;
    for (const auto result : {ExecResult::kExecuted, ExecResult::kAborted}) {
        auto client = std::make_shared<MockClientBase>();
        MockTransaction transaction{client, std::make_unique<ConditionalTransactionMock>(result)};
        auto get = transaction.Get(kKey);
        auto exec = transaction.Exec({}, ExecCondition::IfEq(kKey, kValue));
        if (result == ExecResult::kExecuted) {
            UEXPECT_NO_THROW(exec.Get());
            EXPECT_EQ(get.Get(), kValue);
        } else {
            UEXPECT_THROW(exec.Get(), TransactionAbortedException);
            UEXPECT_THROW(get.Get(), TransactionAbortedException);
        }
    }
}

}  // namespace storages::redis

USERVER_NAMESPACE_END

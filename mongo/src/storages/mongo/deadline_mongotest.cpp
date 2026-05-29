#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testpoint_control.hpp>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/tracing/span.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;
namespace mongo = storages::mongo;

namespace {

class DeadlinePropagation : public MongoPoolFixture {};

server::request::TaskInheritedData MakeRequestData(engine::Deadline deadline) {
    return {{}, "dummy-method", {}, deadline};
}

}  // namespace

UTEST_F(DeadlinePropagation, PoolOverload) {
    auto pool_config = MakeTestPoolConfig();
    pool_config.pool_settings.initial_size = 1;
    pool_config.pool_settings.idle_limit = 1;
    pool_config.pool_settings.max_size = 1;
    pool_config.queue_timeout = 10ms;
    auto pool = MakePool({}, pool_config);

    auto coll = pool.GetCollection("pool_overload");

    UASSERT_NO_THROW(coll.InsertOne(bson::MakeDoc("_id", 1, "foo", 42)));

    // Cursor holds the only available connection.
    auto cursor = coll.Find(bson::MakeDoc("foo", 42));

    const auto deadline = engine::Deadline::FromDuration(5s);
    UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)), mongo::PoolOverloadException);
    EXPECT_FALSE(deadline.IsReached());
}

UTEST_F(DeadlinePropagation, PoolOverloadDeadlinePropagation) {
    auto pool_config = MakeTestPoolConfig();
    pool_config.pool_settings.initial_size = 1;
    pool_config.pool_settings.idle_limit = 1;
    pool_config.pool_settings.max_size = 1;
    pool_config.queue_timeout = utest::kMaxTestWaitTime;
    auto pool = MakePool({}, pool_config);

    auto coll = pool.GetCollection("dp");

    UASSERT_NO_THROW(coll.InsertOne(bson::MakeDoc("_id", 1, "foo", 42)));

    // Cursor holds the only available connection.
    auto cursor = coll.Find(bson::MakeDoc("foo", 42));

    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(500ms)));

    const auto deadline = engine::Deadline::FromDuration(5s);
    UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)), mongo::PoolOverloadException);
    EXPECT_FALSE(deadline.IsReached());
}

UTEST_F(DeadlinePropagation, CancelledByDeadline) {
    auto coll = GetDefaultPool().GetCollection("dp");

    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(-1s)));

    UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)), mongo::CancelledException);
}

UTEST_F(DeadlinePropagation, AlreadyCancelled) {
    auto coll = GetDefaultPool().GetCollection("dp");

    engine::current_task::GetCancellationToken().RequestCancel();

    UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)), mongo::CancelledException);
}

UTEST_F(DeadlinePropagation, CancelledByDeadlineAfterGetClient) {
    auto pool_config = MakeTestPoolConfig();
    pool_config.pool_settings.initial_size = 0;
    pool_config.pool_settings.idle_limit = 1;
    pool_config.pool_settings.max_size = 1;
    pool_config.queue_timeout = utest::kMaxTestWaitTime;

    {
        // check positive case, connection creation is fast
        auto pool = MakePool({}, pool_config);
        auto coll = pool.GetCollection("dp");
        server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(100ms)));
        UASSERT_NO_THROW(coll.InsertOne(bson::MakeDoc("_id", 1)));
    }
    {
        // check negative case, connection creation is too slow
        auto pool = MakePool({}, pool_config);
        auto coll = pool.GetCollection("dp");
        // delay in connection create
        testsuite::TestpointControl testpoint_control;
        class SlowConnectionClient final : public testsuite::TestpointClientBase {
        public:
            mutable size_t times_called = 0;
            ~SlowConnectionClient() override { Unregister(); }
            void Execute(std::string_view, const formats::json::Value&, Callback) const override {
                engine::SleepFor(std::chrono::milliseconds{200});
                times_called++;
            }
        } slow_client;
        testpoint_control.SetClient(slow_client);
        testpoint_control.SetEnabledNames({"mongo-connection-created"});

        server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(100ms)));
        UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)), mongo::CancelledException);
        EXPECT_EQ(slow_client.times_called, 1);
    }
}

USERVER_NAMESPACE_END

#include <userver/utest/utest.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/server/request/task_inherited_data.hpp>

#include <storages/mongo/dynamic_config.hpp>
#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>

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
  mongo::PoolConfig pool_config{};
  pool_config.initial_size = 1;
  pool_config.idle_limit = 1;
  pool_config.max_size = 1;
  pool_config.queue_timeout = 10ms;
  auto pool = MakePool({}, pool_config);

  auto coll = pool.GetCollection("pool_overload");

  UASSERT_NO_THROW(coll.InsertOne(bson::MakeDoc("_id", 1, "foo", 42)));

  // Cursor holds the only available connection.
  auto cursor = coll.Find(bson::MakeDoc("foo", 42));

  const auto deadline = engine::Deadline::FromDuration(5s);
  UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)),
                mongo::PoolOverloadException);
  EXPECT_FALSE(deadline.IsReached());
}

UTEST_F(DeadlinePropagation, PoolOverloadDeadlinePropagation) {
  mongo::PoolConfig pool_config{};
  pool_config.initial_size = 1;
  pool_config.idle_limit = 1;
  pool_config.max_size = 1;
  pool_config.queue_timeout = utest::kMaxTestWaitTime;
  auto pool = MakePool({}, pool_config);

  auto coll = pool.GetCollection("dp");

  UASSERT_NO_THROW(coll.InsertOne(bson::MakeDoc("_id", 1, "foo", 42)));

  // Cursor holds the only available connection.
  auto cursor = coll.Find(bson::MakeDoc("foo", 42));

  server::request::kTaskInheritedData.Set(
      MakeRequestData(engine::Deadline::FromDuration(500ms)));

  const auto deadline = engine::Deadline::FromDuration(5s);
  UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)),
                mongo::PoolOverloadException);
  EXPECT_FALSE(deadline.IsReached());
}

UTEST_F(DeadlinePropagation, CancelledByDeadline) {
  auto coll = GetDefaultPool().GetCollection("dp");

  server::request::kTaskInheritedData.Set(
      MakeRequestData(engine::Deadline::FromDuration(-1s)));

  UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)),
                mongo::CancelledException);
}

UTEST_F(DeadlinePropagation, AlreadyCancelled) {
  auto coll = GetDefaultPool().GetCollection("dp");

  engine::current_task::GetCancellationToken().RequestCancel();

  UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 2)),
                mongo::CancelledException);
}

USERVER_NAMESPACE_END

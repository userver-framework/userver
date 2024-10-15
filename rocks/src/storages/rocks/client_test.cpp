#include <userver/storages/rocks/client.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(Rocks, CheckCRUD) {
  storages::rocks::Client client{"/tmp/rocksdb_simple_example",
                                 engine::current_task::GetTaskProcessor()};

  std::string key = "key";

  std::string res = client.Get(key);
  EXPECT_EQ("", res);

  std::string value = "value";
  client.Put(key, value);
  res = client.Get(key);
  EXPECT_EQ(value, res);

  client.Delete(key);
  res = client.Get(key);
  EXPECT_EQ("", res);

}

UTEST(RocksSnapshot, CheckCRUD) {
  storages::rocks::Client client{"/tmp/rocksdb_simple_snapshot",
                                 engine::current_task::GetTaskProcessor()};
  std::string key = "key";
  std::string value = "value";

  client.Put(key, value);
  std::string res = client.Get(key);
  EXPECT_EQ(value, res);

  storages::rocks::Client snapshot(client.MakeSnapshot("/tmp/snapshot"));
  res = snapshot.Get(key);
  EXPECT_EQ(value, res);


  std::string new_value = "new value";
  snapshot.Put(key, new_value);
  res = snapshot.Get(key);
  EXPECT_EQ(new_value, res);

  res = client.Get(key);
  EXPECT_EQ(value, res);


  snapshot.Delete(key);
  res = snapshot.Get(key);
  EXPECT_EQ("", res);

  res = client.Get(key);
  EXPECT_EQ(value, res);
}

}  // namespace

USERVER_NAMESPACE_END

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

}  // namespace

USERVER_NAMESPACE_END

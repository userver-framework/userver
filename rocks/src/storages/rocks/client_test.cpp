#include <userver/storages/rocks/client.hpp>
#include <userver/storages/rocks/impl/exception.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(Rocks, CheckCRUD) {
  storages::rocks::Client client =
      storages::rocks::Client("/tmp/rocksdb_simple_example");

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

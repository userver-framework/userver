#include <userver/utest/utest.hpp>
#include <userver/storages/rocks/client.hpp>
#include <userver/storages/rocks/impl/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(Rocks, CheckCRUD) {
  auto client = storages::rocks::Client("/tmp/rocksdb_simple_example");

  std::string key = "key";
  auto res = client.Get(key);
  EXPECT_EQ("", res);


  std::string value = "value";
  client.Put(key, value);
  res = client.Get(key);
  EXPECT_EQ(value, res);


  client.Delete(key);
  res = client.Get(key);
  EXPECT_EQ("", res);
}

} // namespace

USERVER_NAMESPACE_END

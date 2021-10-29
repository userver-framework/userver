#include <userver/utest/utest.hpp>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>

USERVER_NAMESPACE_BEGIN

using namespace formats::bson;
using namespace storages::mongo;

namespace {
Pool MakeTestPool(clients::dns::Resolver& dns_resolver) {
  return MakeTestsuiteMongoPool("exception_test", &dns_resolver);
}
}  // namespace

UTEST(Exception, DuplicateKey) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("duplicate_key");

  ASSERT_NO_THROW(coll.InsertOne(MakeDoc("_id", 1)));
  EXPECT_THROW(coll.InsertOne(MakeDoc("_id", 1)), DuplicateKeyException);
}

USERVER_NAMESPACE_END

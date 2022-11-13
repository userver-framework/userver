#include <userver/utest/utest.hpp>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;
namespace mongo = storages::mongo;

namespace {
mongo::Pool MakeTestPool(clients::dns::Resolver& dns_resolver) {
  return MakeTestsuiteMongoPool("exception_test", &dns_resolver);
}
}  // namespace

UTEST(Exception, DuplicateKey) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("duplicate_key");

  UASSERT_NO_THROW(coll.InsertOne(bson::MakeDoc("_id", 1)));
  UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 1)),
                mongo::DuplicateKeyException);
}

USERVER_NAMESPACE_END

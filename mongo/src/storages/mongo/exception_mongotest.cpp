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
class Exception : public MongoPoolFixture {};
}  // namespace

UTEST_P(Exception, DuplicateKey) {
    auto coll = GetDefaultPool().GetCollection("duplicate_key");

    UASSERT_NO_THROW(coll.InsertOne(bson::MakeDoc("_id", 1)));
    UEXPECT_THROW(coll.InsertOne(bson::MakeDoc("_id", 1)), mongo::DuplicateKeyException);
}

INSTANTIATE_UTEST_SUITE_P(
    Driver,
    Exception,
    ::testing::ValuesIn(GetMongoPoolImplementations()),
    GetMongoPoolImplementationName
);

USERVER_NAMESPACE_END

#include <userver/utest/using_namespace_userver.hpp>

/// [Unit test]
#include <userver/storages/mongo/utest/mongo_fixture.hpp>

#include <userver/formats/bson/inline.hpp>

using MongoTest = storages::mongo::utest::MongoTest;

UTEST_F(MongoTest, Sample) {
  auto collection = GetPool()->GetCollection("xyz");
  EXPECT_EQ(0, collection.Count({}));
  collection.InsertOne(formats::bson::MakeDoc("x", 2));
  EXPECT_EQ(1, collection.Count({}));
}
/// [Unit test]

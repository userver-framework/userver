#include <userver/utest/utest.hpp>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo.hpp>

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;
namespace mongo = storages::mongo;

namespace {
class Bulk : public MongoPoolFixture {};
}  // namespace

UTEST_F(Bulk, Empty) {
  auto coll = GetDefaultPool().GetCollection("empty");

  auto bulk = coll.MakeUnorderedBulk();
  EXPECT_TRUE(bulk.IsEmpty());
  auto result = coll.Execute(std::move(bulk));

  EXPECT_EQ(0, result.InsertedCount());
  EXPECT_EQ(0, result.MatchedCount());
  EXPECT_EQ(0, result.ModifiedCount());
  EXPECT_EQ(0, result.UpsertedCount());
  EXPECT_EQ(0, result.DeletedCount());
  EXPECT_TRUE(result.UpsertedIds().empty());
  EXPECT_TRUE(result.ServerErrors().empty());
  EXPECT_TRUE(result.WriteConcernErrors().empty());
}

UTEST_F(Bulk, InsertOne) {
  auto coll = GetDefaultPool().GetCollection("insert_one");

  {
    auto bulk = coll.MakeOrderedBulk();
    bulk.InsertOne(bson::MakeDoc("x", 1));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(1, result.InsertedCount());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto bulk = coll.MakeUnorderedBulk(mongo::options::WriteConcern::kMajority);
    bulk.InsertOne(bson::MakeDoc("x", 1));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(1, result.InsertedCount());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto bulk = coll.MakeOrderedBulk();
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    EXPECT_FALSE(bulk.IsEmpty());
    UEXPECT_THROW(coll.Execute(std::move(bulk)), mongo::DuplicateKeyException);
  }
  coll.DeleteMany({});
  {
    auto bulk =
        coll.MakeOrderedBulk(mongo::options::SuppressServerExceptions{});
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    bulk.InsertOne(bson::MakeDoc("_id", 2));
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    bulk.InsertOne(bson::MakeDoc("_id", 3));
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(2, result.InsertedCount());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());

    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_EQ(11000, errors[2].Code());
  }
  coll.DeleteMany({});
  {
    auto bulk =
        coll.MakeUnorderedBulk(mongo::options::SuppressServerExceptions{});
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    bulk.InsertOne(bson::MakeDoc("_id", 2));
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    bulk.InsertOne(bson::MakeDoc("_id", 3));
    bulk.InsertOne(bson::MakeDoc("_id", 1));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(3, result.InsertedCount());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());

    auto errors = result.ServerErrors();
    ASSERT_EQ(2, errors.size());
    EXPECT_EQ(11000, errors[2].Code());
    EXPECT_EQ(11000, errors[4].Code());
  }
}

UTEST_F(Bulk, ReplaceOne) {
  auto coll = GetDefaultPool().GetCollection("replace_one");

  coll.InsertOne(bson::MakeDoc("_id", 1));
  {
    auto bulk = coll.MakeOrderedBulk();
    bulk.ReplaceOne({}, bson::MakeDoc("x", 2));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(0, result.InsertedCount());
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto bulk =
        coll.MakeUnorderedBulk(mongo::options::WriteConcern::kMajority,
                               mongo::options::SuppressServerExceptions{});
    bulk.ReplaceOne(bson::MakeDoc("y", 0), bson::MakeDoc("_id", 1),
                    mongo::options::Upsert{});
    bulk.ReplaceOne({}, bson::MakeDoc("x", 3));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(0, result.InsertedCount());
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());

    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_EQ(11000, errors[0].Code());
  }
}

UTEST_F(Bulk, Update) {
  auto coll = GetDefaultPool().GetCollection("update");

  {
    auto bulk = coll.MakeOrderedBulk();
    bulk.UpdateOne(bson::MakeDoc("_id", 1),
                   bson::MakeDoc("$set", bson::MakeDoc("x", 1)),
                   mongo::options::Upsert{});
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(0, result.InsertedCount());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());

    auto upserted_ids = result.UpsertedIds();
    EXPECT_EQ(1, upserted_ids.size());
    EXPECT_EQ(1, upserted_ids[0].As<int>());
  }
  {
    auto bulk =
        coll.MakeUnorderedBulk(mongo::options::SuppressServerExceptions{});
    bulk.UpdateOne(bson::MakeDoc("y", 2),
                   bson::MakeDoc("$setOnInsert", bson::MakeDoc("_id", 1)),
                   mongo::options::Upsert{});
    bulk.UpdateOne(bson::MakeDoc("_id", 2),
                   bson::MakeDoc("$set", bson::MakeDoc("x", 2)),
                   mongo::options::Upsert{});
    bulk.UpdateMany({}, bson::MakeDoc("$inc", bson::MakeDoc("x", 1)));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    EXPECT_EQ(0, result.InsertedCount());
    EXPECT_EQ(2, result.MatchedCount());
    EXPECT_EQ(2, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.WriteConcernErrors().empty());

    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_EQ(11000, errors[0].Code());

    auto upserted_ids = result.UpsertedIds();
    EXPECT_EQ(1, upserted_ids.size());
    EXPECT_EQ(2, upserted_ids[1].As<int>());
  }
}

UTEST_F(Bulk, Delete) {
  auto coll = GetDefaultPool().GetCollection("delete");

  {
    std::vector<formats::bson::Document> docs;
    docs.reserve(10);
    for (int i = 0; i < 10; ++i) docs.push_back(bson::MakeDoc("x", i));
    coll.InsertMany(std::move(docs));
  }

  auto bulk = coll.MakeUnorderedBulk();
  bulk.DeleteOne(bson::MakeDoc("x", 1));
  bulk.DeleteOne(bson::MakeDoc("x", bson::MakeDoc("$gt", 6)));
  bulk.DeleteMany(bson::MakeDoc("x", bson::MakeDoc("$gt", 10)));
  bulk.DeleteMany(bson::MakeDoc("x", bson::MakeDoc("$lt", 5)));
  EXPECT_FALSE(bulk.IsEmpty());
  auto result = coll.Execute(std::move(bulk));

  EXPECT_EQ(0, result.InsertedCount());
  EXPECT_EQ(0, result.MatchedCount());
  EXPECT_EQ(0, result.ModifiedCount());
  EXPECT_EQ(0, result.UpsertedCount());
  EXPECT_EQ(6, result.DeletedCount());
  EXPECT_TRUE(result.UpsertedIds().empty());
  EXPECT_TRUE(result.ServerErrors().empty());
  EXPECT_TRUE(result.WriteConcernErrors().empty());
}

UTEST_F(Bulk, Mixed) {
  auto coll = GetDefaultPool().GetCollection("mixed");

  auto bulk = coll.MakeOrderedBulk();
  bulk.InsertOne(bson::MakeDoc("x", 1));
  bulk.InsertOne(bson::MakeDoc("x", 2));
  bulk.InsertOne(bson::MakeDoc("y", 3));
  bulk.UpdateMany(bson::MakeDoc("x", bson::MakeDoc("$exists", true)),
                  bson::MakeDoc("$inc", bson::MakeDoc("x", -1)));
  bulk.ReplaceOne(bson::MakeDoc("y", 3), bson::MakeDoc("x", 2));
  bulk.UpdateOne(bson::MakeDoc("y", 3),
                 bson::MakeDoc("$set", bson::MakeDoc("x", 3)),
                 mongo::options::Upsert{});
  bulk.DeleteMany(bson::MakeDoc("x", bson::MakeDoc("$gt", 1)));
  bulk.UpdateMany({}, bson::MakeDoc("$set", bson::MakeDoc("x", 0)));
  bulk.DeleteOne(bson::MakeDoc("x", bson::MakeDoc("$lt", 1)));
  EXPECT_FALSE(bulk.IsEmpty());
  auto result = coll.Execute(std::move(bulk));

  EXPECT_EQ(3, result.InsertedCount());
  EXPECT_EQ(5, result.MatchedCount());
  EXPECT_EQ(4, result.ModifiedCount());
  EXPECT_EQ(1, result.UpsertedCount());
  EXPECT_EQ(3, result.DeletedCount());
  EXPECT_TRUE(result.ServerErrors().empty());
  EXPECT_TRUE(result.WriteConcernErrors().empty());

  auto upserted_ids = result.UpsertedIds();
  EXPECT_EQ(1, upserted_ids.size());
  EXPECT_TRUE(upserted_ids[5].IsOid());
}

USERVER_NAMESPACE_END

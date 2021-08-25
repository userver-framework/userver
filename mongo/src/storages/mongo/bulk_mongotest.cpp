#include <userver/utest/utest.hpp>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo.hpp>

using namespace formats::bson;
using namespace storages::mongo;

namespace {
Pool MakeTestPool() { return MakeTestsuiteMongoPool("bulk_test"); }
}  // namespace

UTEST(Bulk, Empty) {
  auto pool = MakeTestPool();
  auto coll = pool.GetCollection("empty");

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

UTEST(Bulk, InsertOne) {
  auto pool = MakeTestPool();
  auto coll = pool.GetCollection("insert_one");

  {
    auto bulk = coll.MakeOrderedBulk();
    bulk.InsertOne(MakeDoc("x", 1));
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
    auto bulk = coll.MakeUnorderedBulk(options::WriteConcern::kMajority);
    bulk.InsertOne(MakeDoc("x", 1));
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
    bulk.InsertOne(MakeDoc("_id", 1));
    bulk.InsertOne(MakeDoc("_id", 1));
    EXPECT_FALSE(bulk.IsEmpty());
    EXPECT_THROW(coll.Execute(std::move(bulk)), DuplicateKeyException);
  }
  coll.DeleteMany({});
  {
    auto bulk = coll.MakeOrderedBulk(options::SuppressServerExceptions{});
    bulk.InsertOne(MakeDoc("_id", 1));
    bulk.InsertOne(MakeDoc("_id", 2));
    bulk.InsertOne(MakeDoc("_id", 1));
    bulk.InsertOne(MakeDoc("_id", 3));
    bulk.InsertOne(MakeDoc("_id", 1));
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
    auto bulk = coll.MakeUnorderedBulk(options::SuppressServerExceptions{});
    bulk.InsertOne(MakeDoc("_id", 1));
    bulk.InsertOne(MakeDoc("_id", 2));
    bulk.InsertOne(MakeDoc("_id", 1));
    bulk.InsertOne(MakeDoc("_id", 3));
    bulk.InsertOne(MakeDoc("_id", 1));
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

UTEST(Bulk, ReplaceOne) {
  auto pool = MakeTestPool();
  auto coll = pool.GetCollection("replace_one");

  coll.InsertOne(MakeDoc("_id", 1));
  {
    auto bulk = coll.MakeOrderedBulk();
    bulk.ReplaceOne({}, MakeDoc("x", 2));
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
    auto bulk = coll.MakeUnorderedBulk(options::WriteConcern::kMajority,
                                       options::SuppressServerExceptions{});
    bulk.ReplaceOne(MakeDoc("y", 0), MakeDoc("_id", 1), options::Upsert{});
    bulk.ReplaceOne({}, MakeDoc("x", 3));
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

UTEST(Bulk, Update) {
  auto pool = MakeTestPool();
  auto coll = pool.GetCollection("update");

  {
    auto bulk = coll.MakeOrderedBulk();
    bulk.UpdateOne(MakeDoc("_id", 1), MakeDoc("$set", MakeDoc("x", 1)),
                   options::Upsert{});
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
    auto bulk = coll.MakeUnorderedBulk(options::SuppressServerExceptions{});
    bulk.UpdateOne(MakeDoc("y", 2), MakeDoc("$setOnInsert", MakeDoc("_id", 1)),
                   options::Upsert{});
    bulk.UpdateOne(MakeDoc("_id", 2), MakeDoc("$set", MakeDoc("x", 2)),
                   options::Upsert{});
    bulk.UpdateMany({}, MakeDoc("$inc", MakeDoc("x", 1)));
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

UTEST(Bulk, Delete) {
  auto pool = MakeTestPool();
  auto coll = pool.GetCollection("delete");

  {
    std::vector<formats::bson::Document> docs;
    for (int i = 0; i < 10; ++i) docs.push_back(MakeDoc("x", i));
    coll.InsertMany(std::move(docs));
  }

  auto bulk = coll.MakeUnorderedBulk();
  bulk.DeleteOne(MakeDoc("x", 1));
  bulk.DeleteOne(MakeDoc("x", MakeDoc("$gt", 6)));
  bulk.DeleteMany(MakeDoc("x", MakeDoc("$gt", 10)));
  bulk.DeleteMany(MakeDoc("x", MakeDoc("$lt", 5)));
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

UTEST(Bulk, Mixed) {
  auto pool = MakeTestPool();
  auto coll = pool.GetCollection("mixed");

  auto bulk = coll.MakeOrderedBulk();
  bulk.InsertOne(MakeDoc("x", 1));
  bulk.InsertOne(MakeDoc("x", 2));
  bulk.InsertOne(MakeDoc("y", 3));
  bulk.UpdateMany(MakeDoc("x", MakeDoc("$exists", true)),
                  MakeDoc("$inc", MakeDoc("x", -1)));
  bulk.ReplaceOne(MakeDoc("y", 3), MakeDoc("x", 2));
  bulk.UpdateOne(MakeDoc("y", 3), MakeDoc("$set", MakeDoc("x", 3)),
                 options::Upsert{});
  bulk.DeleteMany(MakeDoc("x", MakeDoc("$gt", 1)));
  bulk.UpdateMany({}, MakeDoc("$set", MakeDoc("x", 0)));
  bulk.DeleteOne(MakeDoc("x", MakeDoc("$lt", 1)));
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

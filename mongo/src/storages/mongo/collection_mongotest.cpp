#include "collection_mongotest.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using namespace formats::bson;
using namespace storages::mongo;

Pool MakeTestPool(clients::dns::Resolver* dns_resolver) {
  return MakeTestsuiteMongoPool("collection_test", dns_resolver);
}

}  // namespace

UTEST(Collection, GetaddrinfoResolver) {
  auto dns_resolver = nullptr;
  auto pool = MakeTestPool(dns_resolver);
  static const auto kFilter = MakeDoc("x", 1);

  auto coll = pool.GetCollection("getaddrinfo");

  EXPECT_EQ(0, coll.CountApprox());
  EXPECT_EQ(0, coll.Count({}));
  EXPECT_EQ(0, coll.Count(kFilter));
}

UTEST(Collection, Read) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  static const auto kFilter = MakeDoc("x", 1);

  {
    auto coll = pool.GetCollection("read");

    EXPECT_EQ(0, coll.CountApprox());
    EXPECT_EQ(0, coll.Count({}));
    EXPECT_EQ(0, coll.Count(kFilter));

    coll.InsertOne(MakeDoc("x", 2));
    EXPECT_EQ(1, coll.CountApprox());
    EXPECT_EQ(1, coll.Count({}));
    EXPECT_EQ(0, coll.Count(kFilter));

    coll.InsertOne(kFilter);
    EXPECT_EQ(2, coll.CountApprox());
    EXPECT_EQ(2, coll.Count({}));
    EXPECT_EQ(1, coll.Count(kFilter));

    coll.InsertOne(MakeDoc("x", 3));
    EXPECT_EQ(3, coll.CountApprox());
    EXPECT_EQ(3, coll.Count({}));
    EXPECT_EQ(1, coll.Count(kFilter));
    EXPECT_EQ(2, coll.Count(MakeDoc("x", MakeDoc("$gt", 1))));

    coll.InsertOne(kFilter);
    EXPECT_EQ(4, coll.CountApprox());
    EXPECT_EQ(4, coll.Count({}));
    EXPECT_EQ(2, coll.Count(kFilter));
    EXPECT_EQ(2, coll.Count(MakeDoc("x", MakeDoc("$gt", 1))));

    auto other_coll = pool.GetCollection("read_other");
    EXPECT_EQ(0, other_coll.CountApprox());
    EXPECT_EQ(0, other_coll.Count({}));
    EXPECT_EQ(0, other_coll.Count(kFilter));

    {
      size_t sum = 0;
      size_t count = 0;
      for (const auto& doc : coll.Find({})) {
        sum += doc["x"].As<size_t>();
        ++count;
      }
      EXPECT_EQ(4, count);
      EXPECT_EQ(7, sum);
    }
    {
      auto cursor = coll.Aggregate(MakeArray(
          MakeDoc("$group", MakeDoc("_id", nullptr, "count", MakeDoc("$sum", 1),
                                    "sum", MakeDoc("$sum", "$x")))));
      auto doc = *cursor.begin();
      EXPECT_EQ(++cursor.begin(), cursor.end());
      EXPECT_EQ(4, doc["count"].As<int>());
      EXPECT_EQ(7, doc["sum"].As<int>());
    }
    {
      Document prev;
      for (const auto& doc : coll.Find(kFilter)) {
        EXPECT_EQ(1, doc["x"].As<int>());
        if (prev.HasMember("_id")) {
          EXPECT_NE(prev["_id"].As<Oid>(), doc["_id"].As<Oid>());
          EXPECT_NE(prev["_id"], doc["_id"]);
        }
        prev = doc;
      }
    }
    {
      auto cursor = coll.Find({});
      EXPECT_TRUE(cursor);
      EXPECT_TRUE(cursor.HasMore());
      for ([[maybe_unused]] const auto& doc : cursor)
        ;  // exhaust
      EXPECT_FALSE(cursor);
      EXPECT_FALSE(cursor.HasMore());
      for ([[maybe_unused]] const auto& doc : cursor) {
        ADD_FAILURE() << "read from exhausted cursor succeeded";
      }
    }
  }

  EXPECT_EQ(4, pool.GetCollection("read").CountApprox());
  EXPECT_EQ(0, pool.GetCollection("read_other").CountApprox());
}

UTEST(Collection, InsertOne) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto coll = pool.GetCollection("insert_one");

  {
    auto result = coll.InsertOne(MakeDoc("_id", 1));
    EXPECT_EQ(1, result.InsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  UEXPECT_THROW(coll.InsertOne(MakeDoc("_id", 1)), DuplicateKeyException);
  {
    auto result =
        coll.InsertOne(MakeDoc("_id", 1), options::SuppressServerExceptions{});
    EXPECT_EQ(0, result.InsertedCount());
    EXPECT_TRUE(result.WriteConcernErrors().empty());

    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_EQ(11000, errors[0].Code());
    EXPECT_TRUE(errors[0].IsServerError());
    UEXPECT_THROW(errors[0].Throw({}), DuplicateKeyException);
  }
}

UTEST(Collection, InsertMany) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto coll = pool.GetCollection("insert_many");

  {
    auto result = coll.InsertMany({});
    EXPECT_EQ(0, result.InsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result = coll.InsertMany({MakeDoc("_id", 1)});
    EXPECT_EQ(1, result.InsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  UEXPECT_THROW(coll.InsertMany({MakeDoc("_id", 2), MakeDoc("_id", 1)}),
                DuplicateKeyException);
  {
    auto result = coll.InsertMany(
        {MakeDoc("_id", 3), MakeDoc("_id", 2), MakeDoc("_id", 1)},
        options::SuppressServerExceptions{});
    EXPECT_EQ(1, result.InsertedCount());
    EXPECT_TRUE(result.WriteConcernErrors().empty());

    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_TRUE(errors[1].IsServerError());
    EXPECT_EQ(11000, errors[1].Code());
  }
}

UTEST(Collection, ReplaceOne) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto coll = pool.GetCollection("replace_one");

  coll.InsertOne(MakeDoc("_id", 1));
  UEXPECT_THROW(
      coll.ReplaceOne(MakeDoc("_id", 1), MakeDoc("$set", MakeDoc("x", 1))),
      InvalidQueryArgumentException);
  {
    auto result = coll.ReplaceOne(MakeDoc("_id", 1), MakeDoc("x", 1));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result = coll.ReplaceOne(MakeDoc("_id", 2), MakeDoc("x", 2));
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result =
        coll.ReplaceOne(MakeDoc("_id", 2), MakeDoc("x", 2), options::Upsert{});
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    auto upserted_ids = result.UpsertedIds();
    ASSERT_TRUE(upserted_ids[0].IsInt32());
    EXPECT_EQ(2, upserted_ids[0].As<int>());
  }
  {
    auto result = coll.ReplaceOne(MakeDoc(), MakeDoc("x", 3));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  EXPECT_EQ(2, coll.CountApprox());
}

UTEST(Collection, Update) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto coll = pool.GetCollection("update");

  coll.InsertOne(MakeDoc("_id", 1));
  UEXPECT_THROW(coll.UpdateOne(MakeDoc("_id", 1), MakeDoc("x", 1)),
                InvalidQueryArgumentException);
  UEXPECT_THROW(coll.UpdateMany(MakeDoc("_id", 1), MakeDoc("x", 1)),
                InvalidQueryArgumentException);

  UpdateOneDoc(coll);

  {
    auto result =
        coll.UpdateOne(MakeDoc("_id", 1), MakeDoc("$set", MakeDoc("x", 10)));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result =
        coll.UpdateOne(MakeDoc("_id", 2), MakeDoc("$set", MakeDoc("x", 20)));
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result =
        coll.UpdateOne(MakeDoc("_id", 2), MakeDoc("$set", MakeDoc("x", 20)),
                       options::Upsert{});
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    auto upserted_ids = result.UpsertedIds();
    ASSERT_TRUE(upserted_ids[0].IsInt32());
    EXPECT_EQ(2, upserted_ids[0].As<int>());
  }
  {
    auto result = coll.UpdateMany(MakeDoc(), MakeDoc("$set", MakeDoc("x", 20)));
    EXPECT_EQ(2, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result =
        coll.UpdateMany(MakeDoc("_id", 3), MakeDoc("$set", MakeDoc("x", 30)),
                        options::Upsert{});
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    auto upserted_ids = result.UpsertedIds();
    ASSERT_TRUE(upserted_ids[0].IsInt32());
    EXPECT_EQ(3, upserted_ids[0].As<int>());
  }
  {
    auto result = coll.UpdateOne(MakeDoc(), MakeDoc("$set", MakeDoc("x", 40)));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  EXPECT_EQ(3, coll.CountApprox());
}

UTEST(Collection, Delete) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto coll = pool.GetCollection("delete");

  {
    std::vector<formats::bson::Document> docs;
    for (int i = 0; i < 10; ++i) docs.push_back(MakeDoc("x", i));
    coll.InsertMany(std::move(docs));
  }

  {
    auto result = coll.DeleteOne(MakeDoc("x", 1));
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    EXPECT_EQ(0, coll.Count(MakeDoc("x", 1)));
  }
  {
    auto result = coll.DeleteOne(MakeDoc("x", MakeDoc("$gt", 6)));
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    EXPECT_EQ(2, coll.Count(MakeDoc("x", MakeDoc("$gt", 6))));
  }
  {
    auto result = coll.DeleteMany(MakeDoc("x", MakeDoc("$gt", 10)));
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result = coll.DeleteMany(MakeDoc("x", MakeDoc("$lt", 5)));
    EXPECT_EQ(4, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  EXPECT_EQ(4, coll.CountApprox());
}

UTEST(Collection, FindAndModify) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto coll = pool.GetCollection("find_and_modify");

  coll.InsertOne(MakeDoc("_id", 1, "x", 10));

  {
    auto result = coll.FindAndModify(MakeDoc("_id", 2), MakeDoc("x", 20));
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_FALSE(result.FoundDocument());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result = coll.FindAndModify(MakeDoc("_id", 2), MakeDoc("x", 20),
                                     options::Upsert{});
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_FALSE(result.FoundDocument());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    auto upserted_ids = result.UpsertedIds();
    ASSERT_TRUE(upserted_ids[0].IsInt32());
    EXPECT_EQ(2, upserted_ids[0].As<int>());
  }
  {
    auto result = coll.FindAndModify(MakeDoc("_id", 3), MakeDoc("x", 30),
                                     options::Upsert{}, options::ReturnNew{});
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    auto upserted_ids = result.UpsertedIds();
    ASSERT_TRUE(upserted_ids[0].IsInt32());
    EXPECT_EQ(3, upserted_ids[0].As<int>());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(doc["_id"], upserted_ids[0]);
    EXPECT_EQ(30, doc["x"].As<int>());
  }
  {
    auto result =
        coll.FindAndModify(MakeDoc("_id", 1), MakeDoc("$inc", MakeDoc("x", 2)));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["_id"].As<int>());
    EXPECT_EQ(10, doc["x"].As<int>());

    EXPECT_EQ(1, coll.Count(MakeDoc("x", 12)));
  }
  {
    auto result =
        coll.FindAndModify(MakeDoc("_id", 1), MakeDoc("$inc", MakeDoc("x", 2)),
                           options::ReturnNew{});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["_id"].As<int>());
    EXPECT_EQ(14, doc["x"].As<int>());
  }
  {
    auto result =
        coll.FindAndModify(MakeDoc(), MakeDoc("$inc", MakeDoc("x", -1)),
                           options::Sort{{"x", options::Sort::kDescending}});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(3, doc["_id"].As<int>());
    EXPECT_EQ(30, doc["x"].As<int>());
  }
  {
    auto result =
        coll.FindAndModify(MakeDoc(), MakeDoc("$inc", MakeDoc("x", -1)),
                           options::Sort{{"x", options::Sort::kAscending}},
                           options::Projection{"y"});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(0, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["_id"].As<int>());
    EXPECT_FALSE(doc.HasMember("x"));
  }
  {
    auto result = coll.FindAndRemove(
        MakeDoc(), options::Sort{{"x", options::Sort::kAscending}},
        options::Projection{"y"});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["_id"].As<int>());
    EXPECT_FALSE(doc.HasMember("x"));
  }
  {
    auto result = coll.FindAndRemove(
        MakeDoc(), options::Sort{{"x", options::Sort::kDescending}},
        options::Projection{}.Exclude("_id"));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_FALSE(doc.HasMember("_id"));
    EXPECT_EQ(29, doc["x"].As<int>());
  }
  coll.FindAndModify(MakeDoc("_id", 1), MakeDoc(), options::Upsert{},
                     options::WriteConcern::kUnacknowledged);
  coll.FindAndRemove(MakeDoc(),
                     options::Sort{{"_id", options::Sort::kDescending}},
                     options::WriteConcern::kUnacknowledged);
  EXPECT_EQ(1, coll.CountApprox());
  EXPECT_EQ(1, coll.Count(MakeDoc("_id", 1)));
}

UTEST(Collection, AggregateOut) {
  auto dns_resolver = MakeDnsResolver();
  SampleMongoPool(MakeTestPool(&dns_resolver));
}

UTEST(Collection, LargeDocRoundtrip) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto coll = pool.GetCollection("large_doc");

  std::string large_string(12 * 1024 * 1024, '\0');
  for (size_t i = 0; i < large_string.size(); ++i) {
    large_string[i] = i % 128;  // Must be UTF-8, ASCII suffices
  }

  coll.InsertOne(MakeDoc("s", large_string));
  auto result = coll.FindOne({});
  ASSERT_TRUE(result);
  EXPECT_EQ(large_string, (*result)["s"].As<std::string>());
}

UTEST(Collection, ExecuteOps) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(&dns_resolver);
  auto mongo_coll = pool.GetCollection("execute_ops");

  mongo_coll.InsertMany({
      formats::bson::MakeDoc("_id", 1, "score", 0, "name", "some_name_0"),
      formats::bson::MakeDoc("_id", 2, "score", 1, "name", "some_name_1"),
      formats::bson::MakeDoc("_id", 3, "score", 2),
      formats::bson::MakeDoc("_id", 4, "score", 3, "name", "some_name_3"),
  });

  EXPECT_EQ(GetWinners(mongo_coll), "some_name_3 <anonymous> some_name_1 ");
}

USERVER_NAMESPACE_END

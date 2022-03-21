#include <algorithm>
#include <chrono>
#include <string>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>

USERVER_NAMESPACE_BEGIN

using namespace formats::bson;
using namespace storages::mongo;

namespace {
Pool MakeTestPool(clients::dns::Resolver& dns_resolver,
                  storages::mongo::Config mongo_config = {}) {
  return MakeTestsuiteMongoPool("options_test", &dns_resolver, mongo_config);
}
}  // namespace

UTEST(Options, ReadPreference) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("read_preference");

  EXPECT_EQ(0, coll.Count({}, options::ReadPreference::kNearest));

  EXPECT_THROW(
      coll.Count({}, options::ReadPreference(options::ReadPreference::kPrimary)
                         .AddTag(MakeDoc("sometag", 1))),
      InvalidQueryArgumentException);

  EXPECT_EQ(0, coll.Count({}, options::ReadPreference(
                                  options::ReadPreference::kSecondaryPreferred)
                                  .SetMaxStaleness(std::chrono::seconds{120})));
  EXPECT_THROW(
      coll.Count({}, options::ReadPreference(options::ReadPreference::kPrimary)
                         .SetMaxStaleness(std::chrono::seconds{120})),
      InvalidQueryArgumentException);
  EXPECT_THROW(coll.Count({}, options::ReadPreference(
                                  options::ReadPreference::kSecondaryPreferred)
                                  .SetMaxStaleness(std::chrono::seconds{-1})),
               InvalidQueryArgumentException);
  EXPECT_THROW(coll.Count({}, options::ReadPreference(
                                  options::ReadPreference::kSecondaryPreferred)
                                  .SetMaxStaleness(std::chrono::seconds{10})),
               InvalidQueryArgumentException);
}

UTEST(Options, ReadConcern) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("read_concern");

  EXPECT_EQ(0, coll.Count({}, options::ReadConcern::kLocal));
  EXPECT_EQ(0, coll.Count({}, options::ReadConcern::kLinearizable));
}

UTEST(Options, SkipLimit) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("skip_limit");

  coll.InsertOne(MakeDoc("x", 0));
  coll.InsertOne(MakeDoc("x", 1));
  coll.InsertOne(MakeDoc("x", 2));
  coll.InsertOne(MakeDoc("x", 3));

  EXPECT_EQ(4, coll.Count({}));
  EXPECT_EQ(4, coll.CountApprox());

  EXPECT_EQ(4, coll.Count({}, options::Skip{0}));
  EXPECT_EQ(4, coll.CountApprox(options::Skip{0}));
  EXPECT_EQ(3, coll.Count({}, options::Skip{1}));
  EXPECT_EQ(3, coll.CountApprox(options::Skip{1}));
  {
    auto cursor = coll.Find({}, options::Skip{2});
    EXPECT_EQ(2, std::distance(cursor.begin(), cursor.end()));
  }

  EXPECT_EQ(4, coll.Count({}, options::Limit{0}));
  EXPECT_EQ(4, coll.CountApprox(options::Limit{0}));
  EXPECT_EQ(2, coll.Count({}, options::Limit{2}));
  EXPECT_EQ(2, coll.CountApprox(options::Limit{2}));
  {
    auto cursor = coll.Find({}, options::Limit{3});
    EXPECT_EQ(3, std::distance(cursor.begin(), cursor.end()));
  }

  EXPECT_EQ(4, coll.Count({}, options::Skip{0}, options::Limit{0}));
  EXPECT_EQ(4, coll.CountApprox(options::Skip{0}, options::Limit{0}));
  EXPECT_EQ(2, coll.Count({}, options::Skip{1}, options::Limit{2}));
  EXPECT_EQ(2, coll.CountApprox(options::Skip{1}, options::Limit{2}));
  {
    auto cursor = coll.Find({}, options::Skip{3}, options::Limit{3});
    EXPECT_EQ(1, std::distance(cursor.begin(), cursor.end()));
  }

  EXPECT_THROW(coll.CountApprox(options::Skip{static_cast<size_t>(-1)}),
               InvalidQueryArgumentException);
  EXPECT_THROW(coll.CountApprox(options::Limit{static_cast<size_t>(-1)}),
               InvalidQueryArgumentException);
}

UTEST(Options, Projection) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("projection");

  coll.InsertOne(MakeDoc("a", 1, "b", "2", "doc", MakeDoc("a", nullptr, "b", 0),
                         "arr", MakeArray(0, 1, 2, 3)));

  {
    auto doc = coll.FindOne({}, options::Projection{});
    ASSERT_TRUE(doc);
    EXPECT_EQ(5, doc->GetSize());
  }
  {
    auto doc = coll.FindOne({}, options::Projection{"_id"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
  }
  {
    auto doc = coll.FindOne({}, options::Projection{"a"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
  }
  {
    auto doc = coll.FindOne(
        {},
        options::Projection{"a"}.Exclude("_id").Include("b").Include("arr"));
    ASSERT_TRUE(doc);
    EXPECT_EQ(3, doc->GetSize());
    EXPECT_TRUE((*doc)["a"].IsInt32());
    EXPECT_TRUE((*doc)["b"].IsString());
    EXPECT_TRUE((*doc)["arr"].IsArray());
  }
  {
    auto doc =
        coll.FindOne({}, options::Projection{}.Exclude("_id").Exclude("doc.a"));
    ASSERT_TRUE(doc);
    EXPECT_EQ(4, doc->GetSize());
    EXPECT_TRUE((*doc)["a"].IsInt32());
    EXPECT_TRUE((*doc)["b"].IsString());
    EXPECT_TRUE((*doc)["arr"].IsArray());
    ASSERT_TRUE((*doc)["doc"].IsDocument());
    EXPECT_EQ(1, (*doc)["doc"].GetSize());
    EXPECT_FALSE((*doc)["doc"].HasMember("a"));
    EXPECT_TRUE((*doc)["doc"]["b"].IsInt32());
  }
  {
    auto doc = coll.FindOne(MakeDoc("arr", MakeDoc("$gt", 0)),
                            options::Projection{"arr.$"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    ASSERT_TRUE((*doc)["arr"].IsArray());
    ASSERT_EQ(1, (*doc)["arr"].GetSize());
    EXPECT_EQ(1, (*doc)["arr"][0].As<int>());
  }
  {
    auto doc = coll.FindOne({}, options::Projection{}.Slice("arr", -1));
    ASSERT_TRUE(doc);
    EXPECT_EQ(5, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
    EXPECT_TRUE((*doc)["b"].IsString());
    EXPECT_TRUE((*doc)["doc"].IsDocument());
    ASSERT_TRUE((*doc)["arr"].IsArray());
    ASSERT_EQ(1, (*doc)["arr"].GetSize());
    EXPECT_EQ(3, (*doc)["arr"][0].As<int>());
  }
  EXPECT_THROW(coll.FindOne({}, options::Projection{}.Slice("arr", -1, 2)),
               InvalidQueryArgumentException);
  {
    auto doc = coll.FindOne({}, options::Projection{"a"}.Slice("arr", 2, -3));
    ASSERT_TRUE(doc);
    EXPECT_EQ(3, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
    ASSERT_TRUE((*doc)["arr"].IsArray());
    ASSERT_EQ(2, (*doc)["arr"].GetSize());
    EXPECT_EQ(1, (*doc)["arr"][0].As<int>());
    EXPECT_EQ(2, (*doc)["arr"][1].As<int>());
  }
  {
    auto doc = coll.FindOne({}, options::Projection{"a"}.ElemMatch("arr", {}));
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
  }
  {
    auto doc = coll.FindOne({}, options::Projection{"a"}.ElemMatch(
                                    "arr", MakeDoc("$bitsAllSet", 2)));
    ASSERT_TRUE(doc);
    EXPECT_EQ(3, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
    ASSERT_TRUE((*doc)["arr"].IsArray());
    ASSERT_EQ(1, (*doc)["arr"].GetSize());
    EXPECT_EQ(2, (*doc)["arr"][0].As<int>());
  }
  {
    auto doc = coll.FindOne({}, options::Projection{"doc.b"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    ASSERT_TRUE((*doc)["doc"].IsDocument());
    EXPECT_EQ(1, (*doc)["doc"].GetSize());
    EXPECT_TRUE((*doc)["doc"]["b"].IsInt32());
  }

  const auto kDummyUpdate = MakeDoc("$set", MakeDoc("a", 1));
  {
    auto result = coll.FindAndModify({}, kDummyUpdate, options::Projection{});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(5, doc.GetSize());
  }
  {
    auto result =
        coll.FindAndModify({}, kDummyUpdate, options::Projection{"_id"});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
  }
  {
    auto result =
        coll.FindAndModify({}, kDummyUpdate, options::Projection{"a"});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(2, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
    EXPECT_TRUE(doc["a"].IsInt32());
  }
  {
    auto result = coll.FindAndModify(
        {}, kDummyUpdate,
        options::Projection{"a"}.Exclude("_id").Include("b").Include("arr"));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(3, doc.GetSize());
    EXPECT_TRUE(doc["a"].IsInt32());
    EXPECT_TRUE(doc["b"].IsString());
    EXPECT_TRUE(doc["arr"].IsArray());
  }
  {
    auto result = coll.FindAndModify(
        {}, kDummyUpdate,
        options::Projection{}.Exclude("_id").Exclude("doc.a"));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(4, doc.GetSize());
    EXPECT_TRUE(doc["a"].IsInt32());
    EXPECT_TRUE(doc["b"].IsString());
    EXPECT_TRUE(doc["arr"].IsArray());
    ASSERT_TRUE(doc["doc"].IsDocument());
    EXPECT_EQ(1, doc["doc"].GetSize());
    EXPECT_FALSE(doc["doc"].HasMember("a"));
    EXPECT_TRUE(doc["doc"]["b"].IsInt32());
  }
  {
    auto result =
        coll.FindAndModify(MakeDoc("arr", MakeDoc("$gt", 0)), kDummyUpdate,
                           options::Projection{"arr.$"});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(2, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
    ASSERT_TRUE(doc["arr"].IsArray());
    ASSERT_EQ(1, doc["arr"].GetSize());
    EXPECT_EQ(1, doc["arr"][0].As<int>());
  }
  {
    auto result = coll.FindAndModify({}, kDummyUpdate,
                                     options::Projection{}.Slice("arr", -1));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(5, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
    EXPECT_TRUE(doc["a"].IsInt32());
    EXPECT_TRUE(doc["b"].IsString());
    EXPECT_TRUE(doc["doc"].IsDocument());
    ASSERT_TRUE(doc["arr"].IsArray());
    ASSERT_EQ(1, doc["arr"].GetSize());
    EXPECT_EQ(3, doc["arr"][0].As<int>());
  }
  EXPECT_THROW(coll.FindAndModify({}, kDummyUpdate,
                                  options::Projection{}.Slice("arr", -1, 2)),
               InvalidQueryArgumentException);
  {
    auto result = coll.FindAndModify(
        {}, kDummyUpdate, options::Projection{"a"}.Slice("arr", 2, -3));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(3, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
    EXPECT_TRUE(doc["a"].IsInt32());
    ASSERT_TRUE(doc["arr"].IsArray());
    ASSERT_EQ(2, doc["arr"].GetSize());
    EXPECT_EQ(1, doc["arr"][0].As<int>());
    EXPECT_EQ(2, doc["arr"][1].As<int>());
  }
  {
    auto result = coll.FindAndModify(
        {}, kDummyUpdate, options::Projection{"a"}.ElemMatch("arr", {}));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(2, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
    EXPECT_TRUE(doc["a"].IsInt32());
  }
  {
    auto result = coll.FindAndModify(
        {}, kDummyUpdate,
        options::Projection{"a"}.ElemMatch("arr", MakeDoc("$bitsAllSet", 2)));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(3, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
    EXPECT_TRUE(doc["a"].IsInt32());
    ASSERT_TRUE(doc["arr"].IsArray());
    ASSERT_EQ(1, doc["arr"].GetSize());
    EXPECT_EQ(2, doc["arr"][0].As<int>());
  }
  {
    auto result =
        coll.FindAndModify({}, kDummyUpdate, options::Projection{"doc.b"});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(2, doc.GetSize());
    EXPECT_TRUE(doc.HasMember("_id"));
    ASSERT_TRUE(doc["doc"].IsDocument());
    EXPECT_EQ(1, doc["doc"].GetSize());
    EXPECT_TRUE(doc["doc"]["b"].IsInt32());
  }
}

UTEST(Options, Sort) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("sort");

  coll.InsertOne(MakeDoc("a", 1, "b", 0));
  coll.InsertOne(MakeDoc("a", 0, "b", 1));

  EXPECT_NO_THROW(coll.FindOne({}, options::Sort{}));
  {
    auto doc =
        coll.FindOne({}, options::Sort({{"a", options::Sort::kAscending}}));
    ASSERT_TRUE(doc);
    EXPECT_EQ(0, (*doc)["a"].As<int>());
    EXPECT_EQ(1, (*doc)["b"].As<int>());
  }
  {
    auto doc =
        coll.FindOne({}, options::Sort{}.By("a", options::Sort::kDescending));
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, (*doc)["a"].As<int>());
    EXPECT_EQ(0, (*doc)["b"].As<int>());
  }
  {
    auto doc =
        coll.FindOne({}, options::Sort{{"b", options::Sort::kAscending}});
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, (*doc)["a"].As<int>());
    EXPECT_EQ(0, (*doc)["b"].As<int>());
  }
  {
    auto doc =
        coll.FindOne({}, options::Sort{{"a", options::Sort::kAscending},
                                       {"b", options::Sort::kAscending}});
    ASSERT_TRUE(doc);
    EXPECT_EQ(0, (*doc)["a"].As<int>());
    EXPECT_EQ(1, (*doc)["b"].As<int>());
  }
  {
    auto doc =
        coll.FindOne({}, options::Sort{{"b", options::Sort::kAscending},
                                       {"a", options::Sort::kAscending}});
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, (*doc)["a"].As<int>());
    EXPECT_EQ(0, (*doc)["b"].As<int>());
  }

  {
    auto result = coll.FindAndRemove({}, options::Sort{});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    coll.InsertOne(*result.FoundDocument());
  }
  {
    auto result = coll.FindAndRemove(
        {}, options::Sort({{"a", options::Sort::kAscending}}));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(0, doc["a"].As<int>());
    EXPECT_EQ(1, doc["b"].As<int>());
    coll.InsertOne(std::move(doc));
  }
  {
    auto result = coll.FindAndRemove(
        {}, options::Sort{}.By("a", options::Sort::kDescending));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["a"].As<int>());
    EXPECT_EQ(0, doc["b"].As<int>());
    coll.InsertOne(std::move(doc));
  }
  {
    auto result =
        coll.FindAndRemove({}, options::Sort{{"b", options::Sort::kAscending}});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["a"].As<int>());
    EXPECT_EQ(0, doc["b"].As<int>());
    coll.InsertOne(std::move(doc));
  }
  {
    auto result =
        coll.FindAndRemove({}, options::Sort{{"a", options::Sort::kAscending},
                                             {"b", options::Sort::kAscending}});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(0, doc["a"].As<int>());
    EXPECT_EQ(1, doc["b"].As<int>());
    coll.InsertOne(std::move(doc));
  }
  {
    auto result =
        coll.FindAndRemove({}, options::Sort{{"b", options::Sort::kAscending},
                                             {"a", options::Sort::kAscending}});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["a"].As<int>());
    EXPECT_EQ(0, doc["b"].As<int>());
    coll.InsertOne(std::move(doc));
  }
}

UTEST(Options, Hint) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("hint");

  EXPECT_NO_THROW(coll.FindOne({}, options::Hint{"some_index"}));
  EXPECT_NO_THROW(coll.FindOne({}, options::Hint{MakeDoc("_id", 1)}));
}

UTEST(Options, AllowPartialResults) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("allow_partial_results");

  EXPECT_NO_THROW(coll.FindOne({}, options::AllowPartialResults{}));
}

UTEST(Options, Tailable) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("tailable");

  EXPECT_NO_THROW(coll.FindOne({}, options::Tailable{}));
}

UTEST(Options, Comment) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("comment");

  EXPECT_NO_THROW(coll.FindOne({}, options::Comment{"snarky comment"}));
}

UTEST(Options, MaxServerTime) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("max_server_time");

  coll.InsertOne(MakeDoc("x", 1));

  EXPECT_NO_THROW(coll.Find(MakeDoc("$where", "sleep(100) || true"),
                            options::MaxServerTime{utest::kMaxTestWaitTime}));
  EXPECT_THROW(coll.Find(MakeDoc("$where", "sleep(100) || true"),
                         options::MaxServerTime{std::chrono::milliseconds{50}}),
               storages::mongo::ServerException);

  EXPECT_NO_THROW(
      coll.FindOne({}, options::MaxServerTime{utest::kMaxTestWaitTime}));
  EXPECT_NO_THROW(
      coll.FindAndRemove({}, options::MaxServerTime{utest::kMaxTestWaitTime}));
}

UTEST(Options, DefaultMaxServerTime) {
  dynamic_config::DocsMap docs_map;
  docs_map.Parse(R"~({"MONGO_DEFAULT_MAX_TIME_MS": 123})~", false);
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver, {docs_map});
  auto coll = pool.GetCollection("max_server_time");

  coll.InsertOne(MakeDoc("x", 1));
  EXPECT_NO_THROW(coll.Find(MakeDoc("$where", "sleep(50) || true")));

  coll.InsertOne(MakeDoc("x", 2));
  coll.InsertOne(MakeDoc("x", 3));
  EXPECT_THROW(coll.Find(MakeDoc("$where", "sleep(50) || true")),
               storages::mongo::ServerException);
  EXPECT_NO_THROW(coll.Find(MakeDoc("$where", "sleep(50) || true"),
                            options::MaxServerTime{utest::kMaxTestWaitTime}));

  EXPECT_NO_THROW(
      coll.FindOne({}, options::MaxServerTime{utest::kMaxTestWaitTime}));
  EXPECT_NO_THROW(
      coll.FindAndRemove({}, options::MaxServerTime{utest::kMaxTestWaitTime}));
}

UTEST(Options, WriteConcern) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("write_concern");

  coll.InsertOne({}, options::WriteConcern::kMajority);
  EXPECT_NO_THROW(coll.InsertOne({}, options::WriteConcern::kMajority));
  EXPECT_NO_THROW(coll.InsertOne({}, options::WriteConcern::kUnacknowledged));
  EXPECT_NO_THROW(coll.InsertOne({}, options::WriteConcern{1}));
  EXPECT_NO_THROW(
      coll.InsertOne({}, options::WriteConcern{options::WriteConcern::kMajority}
                             .SetJournal(false)
                             .SetTimeout(utest::kMaxTestWaitTime)));
  EXPECT_THROW(
      coll.InsertOne({}, options::WriteConcern{static_cast<size_t>(-1)}),
      InvalidQueryArgumentException);
  EXPECT_THROW(coll.InsertOne({}, options::WriteConcern{10}), ServerException);
  EXPECT_THROW(coll.InsertOne({}, options::WriteConcern{"test"}),
               ServerException);

  EXPECT_NO_THROW(coll.FindAndModify({}, {}, options::WriteConcern::kMajority));
  EXPECT_NO_THROW(
      coll.FindAndModify({}, {}, options::WriteConcern::kUnacknowledged));
  EXPECT_NO_THROW(coll.FindAndModify({}, {}, options::WriteConcern{1}));
  EXPECT_NO_THROW(
      coll.FindAndModify({}, {},
                         options::WriteConcern{options::WriteConcern::kMajority}
                             .SetJournal(false)
                             .SetTimeout(utest::kMaxTestWaitTime)));
  EXPECT_THROW(coll.FindAndModify(
                   {}, {}, options::WriteConcern{static_cast<size_t>(-1)}),
               InvalidQueryArgumentException);
  EXPECT_THROW(coll.FindAndModify({}, {}, options::WriteConcern{10}),
               ServerException);
  EXPECT_THROW(coll.FindAndModify({}, {}, options::WriteConcern{"test"}),
               ServerException);
}

UTEST(Options, Unordered) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("unordered");

  coll.InsertOne(MakeDoc("_id", 1));

  operations::InsertMany op({MakeDoc("_id", 1)});
  op.Append(MakeDoc("_id", 2));
  op.SetOption(options::SuppressServerExceptions{});
  {
    auto result = coll.Execute(op);
    EXPECT_EQ(0, result.InsertedCount());
    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_TRUE(errors[0].IsServerError());
    EXPECT_EQ(11000, errors[0].Code());
  }
  op.SetOption(options::Unordered{});
  {
    auto result = coll.Execute(op);
    EXPECT_EQ(1, result.InsertedCount());
    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_TRUE(errors[0].IsServerError());
    EXPECT_EQ(11000, errors[0].Code());
  }
}

UTEST(Options, Upsert) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("upsert");

  coll.InsertOne(MakeDoc("_id", 1));
  {
    auto result = coll.ReplaceOne(MakeDoc("_id", 2), MakeDoc());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result =
        coll.ReplaceOne(MakeDoc("_id", 2), MakeDoc(), options::Upsert{});
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    auto upserted_ids = result.UpsertedIds();
    ASSERT_TRUE(upserted_ids[0].IsInt32());
    EXPECT_EQ(2, upserted_ids[0].As<int>());
  }
  EXPECT_EQ(2, coll.CountApprox());

  {
    auto result = coll.FindAndModify(MakeDoc("_id", 3), MakeDoc());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result =
        coll.FindAndModify(MakeDoc("_id", 3), MakeDoc(), options::Upsert{});
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(1, result.UpsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    auto upserted_ids = result.UpsertedIds();
    ASSERT_TRUE(upserted_ids[0].IsInt32());
    EXPECT_EQ(3, upserted_ids[0].As<int>());
  }
  EXPECT_EQ(3, coll.CountApprox());
}

UTEST(Options, ReturnNew) {
  auto dns_resolver = MakeDnsResolver();
  auto pool = MakeTestPool(dns_resolver);
  auto coll = pool.GetCollection("return_new");

  coll.InsertOne(MakeDoc("_id", 1, "x", 1));
  {
    auto result = coll.FindAndModify(MakeDoc("_id", 1), MakeDoc("x", 2));
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["_id"].As<int>());
    EXPECT_EQ(1, doc["x"].As<int>());
  }
  {
    auto result = coll.FindAndModify(MakeDoc("_id", 1), MakeDoc("x", 3),
                                     options::ReturnNew{});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    auto doc = *result.FoundDocument();
    EXPECT_EQ(1, doc["_id"].As<int>());
    EXPECT_EQ(3, doc["x"].As<int>());
  }
}

USERVER_NAMESPACE_END

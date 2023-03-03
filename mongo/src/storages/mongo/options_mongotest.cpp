#include <algorithm>
#include <chrono>
#include <string>

#include <storages/mongo/dynamic_config.hpp>
#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;
namespace mongo = storages::mongo;

namespace {
class Options : public MongoPoolFixture {};
}  // namespace

UTEST_F(Options, ReadPreference) {
  auto coll = GetDefaultPool().GetCollection("read_preference");

  EXPECT_EQ(0, coll.Count({}, mongo::options::ReadPreference::kNearest));

  UEXPECT_THROW(coll.Count({}, mongo::options::ReadPreference(
                                   mongo::options::ReadPreference::kPrimary)
                                   .AddTag(bson::MakeDoc("sometag", 1))),
                mongo::InvalidQueryArgumentException);

  EXPECT_EQ(
      0, coll.Count({}, mongo::options::ReadPreference(
                            mongo::options::ReadPreference::kSecondaryPreferred)
                            .SetMaxStaleness(std::chrono::seconds{120})));
  UEXPECT_THROW(coll.Count({}, mongo::options::ReadPreference(
                                   mongo::options::ReadPreference::kPrimary)
                                   .SetMaxStaleness(std::chrono::seconds{120})),
                mongo::InvalidQueryArgumentException);
  UEXPECT_THROW(
      coll.Count({}, mongo::options::ReadPreference(
                         mongo::options::ReadPreference::kSecondaryPreferred)
                         .SetMaxStaleness(std::chrono::seconds{-1})),
      mongo::InvalidQueryArgumentException);
  UEXPECT_THROW(
      coll.Count({}, mongo::options::ReadPreference(
                         mongo::options::ReadPreference::kSecondaryPreferred)
                         .SetMaxStaleness(std::chrono::seconds{10})),
      mongo::InvalidQueryArgumentException);
}

UTEST_F(Options, ReadConcern) {
  auto coll = GetDefaultPool().GetCollection("read_concern");

  EXPECT_EQ(0, coll.Count({}, mongo::options::ReadConcern::kLocal));
  EXPECT_EQ(0, coll.Count({}, mongo::options::ReadConcern::kLinearizable));
}

UTEST_F(Options, SkipLimit) {
  auto coll = GetDefaultPool().GetCollection("skip_limit");

  coll.InsertOne(bson::MakeDoc("x", 0));
  coll.InsertOne(bson::MakeDoc("x", 1));
  coll.InsertOne(bson::MakeDoc("x", 2));
  coll.InsertOne(bson::MakeDoc("x", 3));

  EXPECT_EQ(4, coll.Count({}));
  EXPECT_EQ(4, coll.CountApprox());

  EXPECT_EQ(4, coll.Count({}, mongo::options::Skip{0}));
  EXPECT_EQ(4, coll.CountApprox(mongo::options::Skip{0}));
  EXPECT_EQ(3, coll.Count({}, mongo::options::Skip{1}));
  // TODO: not worked with version mongo-c-driver
  // 1.21.1 - repair in https://st.yandex-team.ru/TAXICOMMON-6180
  // EXPECT_EQ(3, coll.CountApprox(mongo::options::Skip{1}));
  {
    auto cursor = coll.Find({}, mongo::options::Skip{2});
    EXPECT_EQ(2, std::distance(cursor.begin(), cursor.end()));
  }

  EXPECT_EQ(4, coll.Count({}, mongo::options::Limit{0}));
  EXPECT_EQ(4, coll.CountApprox(mongo::options::Limit{0}));
  EXPECT_EQ(2, coll.Count({}, mongo::options::Limit{2}));
  // EXPECT_EQ(2, coll.CountApprox(mongo::options::Limit{2}));
  {
    auto cursor = coll.Find({}, mongo::options::Limit{3});
    EXPECT_EQ(3, std::distance(cursor.begin(), cursor.end()));
  }

  EXPECT_EQ(4,
            coll.Count({}, mongo::options::Skip{0}, mongo::options::Limit{0}));
  EXPECT_EQ(
      4, coll.CountApprox(mongo::options::Skip{0}, mongo::options::Limit{0}));
  EXPECT_EQ(2,
            coll.Count({}, mongo::options::Skip{1}, mongo::options::Limit{2}));
  // EXPECT_EQ(
  //    2, coll.CountApprox(mongo::options::Skip{1}, mongo::options::Limit{2}));
  {
    auto cursor =
        coll.Find({}, mongo::options::Skip{3}, mongo::options::Limit{3});
    EXPECT_EQ(1, std::distance(cursor.begin(), cursor.end()));
  }

  UEXPECT_THROW(coll.CountApprox(mongo::options::Skip{static_cast<size_t>(-1)}),
                mongo::InvalidQueryArgumentException);
  UEXPECT_THROW(
      coll.CountApprox(mongo::options::Limit{static_cast<size_t>(-1)}),
      mongo::InvalidQueryArgumentException);
}

UTEST_F(Options, Projection) {
  auto coll = GetDefaultPool().GetCollection("projection");

  coll.InsertOne(bson::MakeDoc("a", 1, "b", "2", "doc",
                               bson::MakeDoc("a", nullptr, "b", 0), "arr",
                               bson::MakeArray(0, 1, 2, 3)));

  {
    auto doc = coll.FindOne({}, mongo::options::Projection{});
    ASSERT_TRUE(doc);
    EXPECT_EQ(5, doc->GetSize());
  }
  {
    auto doc = coll.FindOne({}, mongo::options::Projection{"_id"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
  }
  {
    auto doc = coll.FindOne({}, mongo::options::Projection{"a"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
  }
  {
    auto doc = coll.FindOne(
        {}, mongo::options::Projection{"a"}.Exclude("_id").Include("b").Include(
                "arr"));
    ASSERT_TRUE(doc);
    EXPECT_EQ(3, doc->GetSize());
    EXPECT_TRUE((*doc)["a"].IsInt32());
    EXPECT_TRUE((*doc)["b"].IsString());
    EXPECT_TRUE((*doc)["arr"].IsArray());
  }
  {
    auto doc = coll.FindOne(
        {}, mongo::options::Projection{}.Exclude("_id").Exclude("doc.a"));
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
    auto doc = coll.FindOne(bson::MakeDoc("arr", bson::MakeDoc("$gt", 0)),
                            mongo::options::Projection{"arr.$"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    ASSERT_TRUE((*doc)["arr"].IsArray());
    ASSERT_EQ(1, (*doc)["arr"].GetSize());
    EXPECT_EQ(1, (*doc)["arr"][0].As<int>());
  }
  {
    auto doc = coll.FindOne({}, mongo::options::Projection{}.Slice("arr", -1));
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
  UEXPECT_THROW(
      coll.FindOne({}, mongo::options::Projection{}.Slice("arr", -1, 2)),
      mongo::InvalidQueryArgumentException);
  {
    auto doc =
        coll.FindOne({}, mongo::options::Projection{"a"}.Slice("arr", 2, -3));
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
    auto doc =
        coll.FindOne({}, mongo::options::Projection{"a"}.ElemMatch("arr", {}));
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
  }
  {
    auto doc = coll.FindOne({}, mongo::options::Projection{"a"}.ElemMatch(
                                    "arr", bson::MakeDoc("$bitsAllSet", 2)));
    ASSERT_TRUE(doc);
    EXPECT_EQ(3, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    EXPECT_TRUE((*doc)["a"].IsInt32());
    ASSERT_TRUE((*doc)["arr"].IsArray());
    ASSERT_EQ(1, (*doc)["arr"].GetSize());
    EXPECT_EQ(2, (*doc)["arr"][0].As<int>());
  }
  {
    auto doc = coll.FindOne({}, mongo::options::Projection{"doc.b"});
    ASSERT_TRUE(doc);
    EXPECT_EQ(2, doc->GetSize());
    EXPECT_TRUE(doc->HasMember("_id"));
    ASSERT_TRUE((*doc)["doc"].IsDocument());
    EXPECT_EQ(1, (*doc)["doc"].GetSize());
    EXPECT_TRUE((*doc)["doc"]["b"].IsInt32());
  }
}

UTEST_F(Options, ProjectionTwo) {
  auto coll = GetDefaultPool().GetCollection("projection");

  coll.InsertOne(bson::MakeDoc("a", 1, "b", "2", "doc",
                               bson::MakeDoc("a", nullptr, "b", 0), "arr",
                               bson::MakeArray(0, 1, 2, 3)));

  const auto kDummyUpdate = bson::MakeDoc("$set", bson::MakeDoc("a", 1));
  {
    auto result =
        coll.FindAndModify({}, kDummyUpdate, mongo::options::Projection{});
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
        coll.FindAndModify({}, kDummyUpdate, mongo::options::Projection{"_id"});
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
        coll.FindAndModify({}, kDummyUpdate, mongo::options::Projection{"a"});
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
        mongo::options::Projection{"a"}.Exclude("_id").Include("b").Include(
            "arr"));
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
        mongo::options::Projection{}.Exclude("_id").Exclude("doc.a"));
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
}

UTEST_F(Options, ProjectionThree) {
  auto coll = GetDefaultPool().GetCollection("projection");

  coll.InsertOne(bson::MakeDoc("a", 1, "b", "2", "doc",
                               bson::MakeDoc("a", nullptr, "b", 0), "arr",
                               bson::MakeArray(0, 1, 2, 3)));

  const auto kDummyUpdate = bson::MakeDoc("$set", bson::MakeDoc("a", 1));

  {
    auto result =
        coll.FindAndModify(bson::MakeDoc("arr", bson::MakeDoc("$gt", 0)),
                           kDummyUpdate, mongo::options::Projection{"arr.$"});
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
    auto result = coll.FindAndModify(
        {}, kDummyUpdate, mongo::options::Projection{}.Slice("arr", -1));
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
  UEXPECT_THROW(
      coll.FindAndModify({}, kDummyUpdate,
                         mongo::options::Projection{}.Slice("arr", -1, 2)),
      mongo::InvalidQueryArgumentException);
  {
    auto result = coll.FindAndModify(
        {}, kDummyUpdate, mongo::options::Projection{"a"}.Slice("arr", 2, -3));
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
        {}, kDummyUpdate, mongo::options::Projection{"a"}.ElemMatch("arr", {}));
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
    auto result =
        coll.FindAndModify({}, kDummyUpdate,
                           mongo::options::Projection{"a"}.ElemMatch(
                               "arr", bson::MakeDoc("$bitsAllSet", 2)));
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
    auto result = coll.FindAndModify({}, kDummyUpdate,
                                     mongo::options::Projection{"doc.b"});
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

UTEST_F(Options, Sort) {
  auto coll = GetDefaultPool().GetCollection("sort");

  coll.InsertOne(bson::MakeDoc("a", 1, "b", 0));
  coll.InsertOne(bson::MakeDoc("a", 0, "b", 1));

  UEXPECT_NO_THROW(coll.FindOne({}, mongo::options::Sort{}));
  {
    auto doc = coll.FindOne(
        {}, mongo::options::Sort({{"a", mongo::options::Sort::kAscending}}));
    ASSERT_TRUE(doc);
    EXPECT_EQ(0, (*doc)["a"].As<int>());
    EXPECT_EQ(1, (*doc)["b"].As<int>());
  }
  {
    auto doc = coll.FindOne(
        {}, mongo::options::Sort{}.By("a", mongo::options::Sort::kDescending));
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, (*doc)["a"].As<int>());
    EXPECT_EQ(0, (*doc)["b"].As<int>());
  }
  {
    auto doc = coll.FindOne(
        {}, mongo::options::Sort{{"b", mongo::options::Sort::kAscending}});
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, (*doc)["a"].As<int>());
    EXPECT_EQ(0, (*doc)["b"].As<int>());
  }
  {
    auto doc = coll.FindOne(
        {}, mongo::options::Sort{{"a", mongo::options::Sort::kAscending},
                                 {"b", mongo::options::Sort::kAscending}});
    ASSERT_TRUE(doc);
    EXPECT_EQ(0, (*doc)["a"].As<int>());
    EXPECT_EQ(1, (*doc)["b"].As<int>());
  }
  {
    auto doc = coll.FindOne(
        {}, mongo::options::Sort{{"b", mongo::options::Sort::kAscending},
                                 {"a", mongo::options::Sort::kAscending}});
    ASSERT_TRUE(doc);
    EXPECT_EQ(1, (*doc)["a"].As<int>());
    EXPECT_EQ(0, (*doc)["b"].As<int>());
  }

  {
    auto result = coll.FindAndRemove({}, mongo::options::Sort{});
    EXPECT_EQ(1, result.MatchedCount());
    EXPECT_EQ(1, result.DeletedCount());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
    ASSERT_TRUE(result.FoundDocument());
    coll.InsertOne(*result.FoundDocument());
  }
  {
    auto result = coll.FindAndRemove(
        {}, mongo::options::Sort({{"a", mongo::options::Sort::kAscending}}));
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
        {}, mongo::options::Sort{}.By("a", mongo::options::Sort::kDescending));
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
    auto result = coll.FindAndRemove(
        {}, mongo::options::Sort{{"b", mongo::options::Sort::kAscending}});
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
    auto result = coll.FindAndRemove(
        {}, mongo::options::Sort{{"a", mongo::options::Sort::kAscending},
                                 {"b", mongo::options::Sort::kAscending}});
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
        {}, mongo::options::Sort{{"b", mongo::options::Sort::kAscending},
                                 {"a", mongo::options::Sort::kAscending}});
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

UTEST_F(Options, Hint) {
  auto coll = GetDefaultPool().GetCollection("hint");

  UEXPECT_NO_THROW(coll.FindOne({}, mongo::options::Hint{"some_index"}));
  UEXPECT_NO_THROW(
      coll.FindOne({}, mongo::options::Hint{bson::MakeDoc("_id", 1)}));
}

UTEST_F(Options, AllowPartialResults) {
  auto coll = GetDefaultPool().GetCollection("allow_partial_results");

  UEXPECT_NO_THROW(coll.FindOne({}, mongo::options::AllowPartialResults{}));
}

UTEST_F(Options, Tailable) {
  auto coll = GetDefaultPool().GetCollection("tailable");

  UEXPECT_NO_THROW(coll.FindOne({}, mongo::options::Tailable{}));
}

UTEST_F(Options, Comment) {
  auto coll = GetDefaultPool().GetCollection("comment");

  UEXPECT_NO_THROW(coll.FindOne({}, mongo::options::Comment{"snarky comment"}));
}

UTEST_F(Options, MaxServerTime) {
  auto coll = GetDefaultPool().GetCollection("max_server_time");

  coll.InsertOne(bson::MakeDoc("x", 1));

  UEXPECT_NO_THROW(
      coll.Find(bson::MakeDoc("$where", "sleep(100) || true"),
                mongo::options::MaxServerTime{utest::kMaxTestWaitTime}));
  UEXPECT_THROW(
      coll.Find(bson::MakeDoc("$where", "sleep(100) || true"),
                mongo::options::MaxServerTime{std::chrono::milliseconds{50}}),
      storages::mongo::ServerException);

  UEXPECT_NO_THROW(
      coll.FindOne({}, mongo::options::MaxServerTime{utest::kMaxTestWaitTime}));
  UEXPECT_NO_THROW(coll.FindAndRemove(
      {}, mongo::options::MaxServerTime{utest::kMaxTestWaitTime}));
}

UTEST_F(Options, DefaultMaxServerTime) {
  SetDynamicConfig({{mongo::kDefaultMaxTime, std::chrono::milliseconds{123}}});
  auto coll = GetDefaultPool().GetCollection("max_server_time");

  coll.InsertOne(bson::MakeDoc("x", 1));
  UEXPECT_NO_THROW(coll.Find(bson::MakeDoc("$where", "sleep(50) || true")));

  coll.InsertOne(bson::MakeDoc("x", 2));
  coll.InsertOne(bson::MakeDoc("x", 3));
  UEXPECT_THROW(coll.Find(bson::MakeDoc("$where", "sleep(50) || true")),
                storages::mongo::ServerException);
  UEXPECT_NO_THROW(
      coll.Find(bson::MakeDoc("$where", "sleep(50) || true"),
                mongo::options::MaxServerTime{utest::kMaxTestWaitTime}));

  UEXPECT_NO_THROW(
      coll.FindOne({}, mongo::options::MaxServerTime{utest::kMaxTestWaitTime}));
  UEXPECT_NO_THROW(coll.FindAndRemove(
      {}, mongo::options::MaxServerTime{utest::kMaxTestWaitTime}));
}

UTEST_F(Options, WriteConcern) {
  auto coll = GetDefaultPool().GetCollection("write_concern");

  coll.InsertOne({}, mongo::options::WriteConcern::kMajority);
  UEXPECT_NO_THROW(coll.InsertOne({}, mongo::options::WriteConcern::kMajority));
  UEXPECT_NO_THROW(
      coll.InsertOne({}, mongo::options::WriteConcern::kUnacknowledged));
  UEXPECT_NO_THROW(coll.InsertOne({}, mongo::options::WriteConcern{1}));
  UEXPECT_NO_THROW(coll.InsertOne(
      {}, mongo::options::WriteConcern{mongo::options::WriteConcern::kMajority}
              .SetJournal(false)
              .SetTimeout(utest::kMaxTestWaitTime)));
  UEXPECT_THROW(
      coll.InsertOne({}, mongo::options::WriteConcern{static_cast<size_t>(-1)}),
      mongo::InvalidQueryArgumentException);
  UEXPECT_THROW(coll.InsertOne({}, mongo::options::WriteConcern{10}),
                mongo::ServerException);
  UEXPECT_THROW(coll.InsertOne({}, mongo::options::WriteConcern{"test"}),
                mongo::ServerException);

  UEXPECT_NO_THROW(
      coll.FindAndModify({}, {}, mongo::options::WriteConcern::kMajority));
  UEXPECT_NO_THROW(coll.FindAndModify(
      {}, {}, mongo::options::WriteConcern::kUnacknowledged));
  UEXPECT_NO_THROW(coll.FindAndModify({}, {}, mongo::options::WriteConcern{1}));
  UEXPECT_NO_THROW(coll.FindAndModify(
      {}, {},
      mongo::options::WriteConcern{mongo::options::WriteConcern::kMajority}
          .SetJournal(false)
          .SetTimeout(utest::kMaxTestWaitTime)));
  UEXPECT_THROW(
      coll.FindAndModify({}, {},
                         mongo::options::WriteConcern{static_cast<size_t>(-1)}),
      mongo::InvalidQueryArgumentException);
  UEXPECT_THROW(coll.FindAndModify({}, {}, mongo::options::WriteConcern{10}),
                mongo::ServerException);
  UEXPECT_THROW(
      coll.FindAndModify({}, {}, mongo::options::WriteConcern{"test"}),
      mongo::ServerException);
}

UTEST_F(Options, Unordered) {
  auto coll = GetDefaultPool().GetCollection("unordered");

  coll.InsertOne(bson::MakeDoc("_id", 1));

  mongo::operations::InsertMany op({bson::MakeDoc("_id", 1)});
  op.Append(bson::MakeDoc("_id", 2));
  op.SetOption(mongo::options::SuppressServerExceptions{});
  {
    auto result = coll.Execute(op);
    EXPECT_EQ(0, result.InsertedCount());
    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_TRUE(errors[0].IsServerError());
    EXPECT_EQ(11000, errors[0].Code());
  }
  op.SetOption(mongo::options::Unordered{});
  {
    auto result = coll.Execute(op);
    EXPECT_EQ(1, result.InsertedCount());
    auto errors = result.ServerErrors();
    ASSERT_EQ(1, errors.size());
    EXPECT_TRUE(errors[0].IsServerError());
    EXPECT_EQ(11000, errors[0].Code());
  }
}

UTEST_F(Options, Upsert) {
  auto coll = GetDefaultPool().GetCollection("upsert");

  coll.InsertOne(bson::MakeDoc("_id", 1));
  {
    auto result = coll.ReplaceOne(bson::MakeDoc("_id", 2), bson::MakeDoc());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result = coll.ReplaceOne(bson::MakeDoc("_id", 2), bson::MakeDoc(),
                                  mongo::options::Upsert{});
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
    auto result = coll.FindAndModify(bson::MakeDoc("_id", 3), bson::MakeDoc());
    EXPECT_EQ(0, result.MatchedCount());
    EXPECT_EQ(0, result.ModifiedCount());
    EXPECT_EQ(0, result.UpsertedCount());
    EXPECT_TRUE(result.UpsertedIds().empty());
    EXPECT_TRUE(result.ServerErrors().empty());
    EXPECT_TRUE(result.WriteConcernErrors().empty());
  }
  {
    auto result = coll.FindAndModify(bson::MakeDoc("_id", 3), bson::MakeDoc(),
                                     mongo::options::Upsert{});
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

UTEST_F(Options, ReturnNew) {
  auto coll = GetDefaultPool().GetCollection("return_new");

  coll.InsertOne(bson::MakeDoc("_id", 1, "x", 1));
  {
    auto result =
        coll.FindAndModify(bson::MakeDoc("_id", 1), bson::MakeDoc("x", 2));
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
    auto result =
        coll.FindAndModify(bson::MakeDoc("_id", 1), bson::MakeDoc("x", 3),
                           mongo::options::ReturnNew{});
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

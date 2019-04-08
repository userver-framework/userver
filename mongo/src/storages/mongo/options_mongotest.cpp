#include <utest/utest.hpp>

#include <algorithm>
#include <chrono>
#include <string>

#include <formats/bson.hpp>
#include <storages/mongo/collection.hpp>
#include <storages/mongo/exception.hpp>
#include <storages/mongo/pool.hpp>
#include <storages/mongo/pool_config.hpp>

using namespace formats::bson;
using namespace storages::mongo;

namespace {
Pool MakeTestPool() {
  return {"options_test", "mongodb://localhost:27217/options_test",
          PoolConfig("userver_options_test")};
}
}  // namespace

TEST(Options, ReadPreference) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("read_preference");

    EXPECT_EQ(0, coll.Count({}, options::ReadPreference::kNearest));

    EXPECT_THROW(coll.Count({}, options::ReadPreference(
                                    options::ReadPreference::kPrimary)
                                    .AddTag(MakeDoc("sometag", 1))),
                 InvalidQueryArgumentException);
  });
}

TEST(Options, ReadConcern) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("read_concern");

    EXPECT_EQ(0, coll.Count({}, options::ReadConcern::kLocal));
    EXPECT_EQ(0, coll.Count({}, options::ReadConcern::kLinearizable));
  });
}

TEST(Options, SkipLimit) {
  RunInCoro([] {
    auto pool = MakeTestPool();
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
  });
}

TEST(Options, Projection) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("projection");

    coll.InsertOne(MakeDoc("a", 1, "b", "2", "doc",
                           MakeDoc("a", nullptr, "b", 0), "arr",
                           MakeArray(0, 1, 2, 3)));

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
      auto doc = coll.FindOne(
          {}, options::Projection{}.Exclude("_id").Exclude("doc.a"));
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
      auto doc =
          coll.FindOne({}, options::Projection{"a"}.ElemMatch("arr", {}));
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
      auto doc =
          coll.FindOne({}, options::Projection{MakeDoc("b", 1, "_id", 0)});
      ASSERT_TRUE(doc);
      EXPECT_EQ(1, doc->GetSize());
      EXPECT_TRUE((*doc)["b"].IsString());
    }
  });
}

TEST(Options, Sort) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("sort");

    coll.InsertOne(MakeDoc("a", 1, "b", 0));
    coll.InsertOne(MakeDoc("a", 0, "b", 1));

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
  });
}

TEST(Options, Hint) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("hint");

    EXPECT_NO_THROW(coll.FindOne({}, options::Hint{"some_index"}));
    EXPECT_NO_THROW(coll.FindOne({}, options::Hint{MakeDoc("_id", 1)}));
  });
}

TEST(Options, AllowPartialResults) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("allow_partial_results");

    EXPECT_NO_THROW(coll.FindOne({}, options::AllowPartialResults{}));
  });
}

TEST(Options, Tailable) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("tailable");

    EXPECT_NO_THROW(coll.FindOne({}, options::Tailable{}));
  });
}

TEST(Options, Comment) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("comment");

    EXPECT_NO_THROW(coll.FindOne({}, options::Comment{"snarky comment"}));
  });
}

TEST(Options, MaxServerTime) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("max_server_time");

    EXPECT_NO_THROW(
        coll.FindOne({}, options::MaxServerTime{std::chrono::seconds{1}}));
  });
}

TEST(Options, WriteConcern) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("write_concern");

    coll.InsertOne({}, options::WriteConcern::kMajority);
    EXPECT_NO_THROW(coll.InsertOne({}, options::WriteConcern::kMajority));
    EXPECT_NO_THROW(coll.InsertOne({}, options::WriteConcern::kUnacknowledged));
    EXPECT_NO_THROW(coll.InsertOne({}, options::WriteConcern{1}));
    EXPECT_NO_THROW(coll.InsertOne(
        {}, options::WriteConcern{options::WriteConcern::kMajority}
                .SetJournal(false)
                .SetTimeout(std::chrono::milliseconds(100))));
    EXPECT_THROW(
        coll.InsertOne({}, options::WriteConcern{static_cast<size_t>(-1)}),
        InvalidQueryArgumentException);
    EXPECT_THROW(coll.InsertOne({}, options::WriteConcern{1}.SetTimeout(
                                        std::chrono::milliseconds::max())),
                 InvalidQueryArgumentException);
    EXPECT_THROW(coll.InsertOne({}, options::WriteConcern{10}),
                 ServerException);
    EXPECT_THROW(coll.InsertOne({}, options::WriteConcern{"test"}),
                 ServerException);
  });
}

TEST(Options, Unordered) {
  RunInCoro([] {
    auto pool = MakeTestPool();
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
  });
}

TEST(Options, Upsert) {
  RunInCoro([] {
    auto pool = MakeTestPool();
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
  });
}

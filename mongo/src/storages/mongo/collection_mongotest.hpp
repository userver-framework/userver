#include <userver/utest/utest.hpp>

#include <string>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/storages/mongo/pool.hpp>

USERVER_NAMESPACE_BEGIN

/// [Sample Mongo usage]
inline void SampleMongoPool(storages::mongo::Pool pool) {
  using formats::bson::MakeArray;
  using formats::bson::MakeDoc;

  auto in_coll = pool.GetCollection("aggregate_in");

  in_coll.InsertMany({
      formats::bson::MakeDoc("_id", 1, "x", 0),
      formats::bson::MakeDoc("_id", 2, "x", 1),
      formats::bson::MakeDoc("_id", 3, "x", 2),
  });

  auto cursor = in_coll.Aggregate(
      MakeArray(MakeDoc("$match", MakeDoc("_id", MakeDoc("$gte", 2))),
                MakeDoc("$addFields", MakeDoc("check", true)),
                MakeDoc("$out", "aggregate_out")),
      storages::mongo::options::ReadPreference::kSecondaryPreferred,
      storages::mongo::options::WriteConcern::kMajority);
  EXPECT_FALSE(cursor);

  auto out_coll = pool.GetCollection("aggregate_out");
  EXPECT_EQ(2, out_coll.CountApprox());
  for (const auto& doc : out_coll.Find({})) {
    EXPECT_EQ(doc["_id"].As<int>(), doc["x"].As<int>() + 1);
    EXPECT_TRUE(doc["check"].As<bool>());
  }
}
/// [Sample Mongo usage]

/// [Sample Mongo write result]
inline void UpdateOneDoc(storages::mongo::Collection& coll) {
  using formats::bson::MakeDoc;
  auto result =
      coll.UpdateOne(MakeDoc("_id", 1), MakeDoc("$set", MakeDoc("x", 10)));

  EXPECT_EQ(1, result.MatchedCount());
  EXPECT_EQ(1, result.ModifiedCount());
  EXPECT_EQ(0, result.UpsertedCount());
  EXPECT_TRUE(result.UpsertedIds().empty());
  EXPECT_TRUE(result.ServerErrors().empty());
  EXPECT_TRUE(result.WriteConcernErrors().empty());
}
/// [Sample Mongo write result]

/// [Sample Mongo packaged operation]
inline const storages::mongo::operations::Find& Top3ScoresOp() {
  static const auto kGetTop = [] {
    storages::mongo::operations::Find op({});

    op.SetOption(storages::mongo::options::Sort{{
        "score",
        storages::mongo::options::Sort::kDescending,
    }});
    op.SetOption(storages::mongo::options::Limit{3});

    return op;
  }();

  return kGetTop;
}

inline std::string GetWinners(storages::mongo::Collection& mongo_coll) {
  std::string result;

  for (const auto& doc : mongo_coll.Execute(Top3ScoresOp())) {
    result += doc["name"].As<std::string>("<anonymous>") + ' ';
  }

  return result;
}
/// [Sample Mongo packaged operation]

USERVER_NAMESPACE_END

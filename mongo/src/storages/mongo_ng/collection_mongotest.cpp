#include <utest/utest.hpp>

#include <string>

#include <formats/bson.hpp>
#include <storages/mongo_ng/collection.hpp>
#include <storages/mongo_ng/pool.hpp>
#include <storages/mongo_ng/pool_config.hpp>

using namespace formats::bson;
using namespace storages::mongo_ng;

TEST(Collection, SmokeTemp) {
  RunInCoro([] {
    Pool test_pool("collection_test",
                   "mongodb://localhost:27217/collection_test",
                   PoolConfig("userver_collection_test"));

    static const auto kFilter = MakeDoc("x", 1);

    {
      auto coll = test_pool.GetCollection("collection_test");

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

      auto other_coll = test_pool.GetCollection("other");
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
        for ([[maybe_unused]] const auto& doc : cursor)
          ;  // exhaust
        for ([[maybe_unused]] const auto& doc : cursor) {
          ADD_FAILURE() << "read from exhausted cursor succeeded";
        }
      }
    }

    EXPECT_EQ(4, test_pool.GetCollection("collection_test").CountApprox());
    EXPECT_EQ(0, test_pool.GetCollection("even_more_other").CountApprox());
  });
}

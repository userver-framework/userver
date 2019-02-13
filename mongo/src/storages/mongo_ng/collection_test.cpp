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

      auto other_coll = test_pool.GetCollection("other");
      EXPECT_EQ(0, other_coll.CountApprox());
      EXPECT_EQ(0, other_coll.Count({}));
      EXPECT_EQ(0, other_coll.Count(kFilter));
    }

    EXPECT_EQ(3, test_pool.GetCollection("collection_test").CountApprox());
    EXPECT_EQ(0, test_pool.GetCollection("even_more_other").CountApprox());
  });
}

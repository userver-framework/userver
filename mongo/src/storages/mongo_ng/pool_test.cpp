#include <utest/utest.hpp>

#include <string>

#include <storages/mongo_ng/exception.hpp>
#include <storages/mongo_ng/pool.hpp>
#include <storages/mongo_ng/pool_config.hpp>

using namespace storages::mongo_ng;

namespace {
const PoolConfig kPoolConfig("userver_pool_test");
}  // namespace

TEST(Pool, CollectionAccess) {
  RunInCoro([] {
    // this database always exists
    Pool adminPool("admin", "mongodb://localhost:27217/admin", kPoolConfig);
    // this one should not exist
    Pool testPool("pool_test", "mongodb://localhost:27217/pool_test",
                  kPoolConfig);

    static const std::string kSysVerCollName = "system.version";
    static const std::string kNonexistentCollName = "nonexistent";

    EXPECT_TRUE(adminPool.HasCollection(kSysVerCollName));
    EXPECT_NO_THROW(adminPool.GetCollection(kSysVerCollName));

    EXPECT_FALSE(testPool.HasCollection(kSysVerCollName));
    EXPECT_NO_THROW(testPool.GetCollection(kSysVerCollName));

    EXPECT_FALSE(adminPool.HasCollection(kNonexistentCollName));
    EXPECT_NO_THROW(adminPool.GetCollection(kNonexistentCollName));

    EXPECT_FALSE(testPool.HasCollection(kNonexistentCollName));
    EXPECT_NO_THROW(testPool.GetCollection(kNonexistentCollName));
  });
}

TEST(Pool, ConnectionFailure) {
  RunInCoro([] {
    // constructor should not throw
    Pool badPool("bad", "mongodb://%2Fnonexistent.sock/bad", kPoolConfig);
    EXPECT_THROW(badPool.HasCollection("test"), PoolException);
  });
}

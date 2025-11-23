#include <optional>

#include <userver/storages/rocks/db.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// TAXICOMMON-10374
UTEST(Rocks, DISABLED_CheckCRUD) {
    storages::rocks::Db db{"/tmp/rocksdb_simple_example", 4, engine::current_task::GetTaskProcessor()};

    const std::string key = "key";
    std::optional<std::string> result = db.Get(key);
    EXPECT_EQ(false, result.has_value());

    db.Put(key, "value");
    result = db.Get(key);
    EXPECT_EQ("value", result.value_or(""));

    db.Delete(key);
    result = db.Get(key);
    EXPECT_EQ("", result.value_or(""));
}

}  // namespace

USERVER_NAMESPACE_END

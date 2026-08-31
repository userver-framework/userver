#include <userver/storages/rocks/client.hpp>

#include <rocksdb/db.h>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using fs::blocking::TempDirectory;
using storages::rocks::Exception;
using storages::rocks::Snapshot;

UTEST(Rocks, CheckCRUD) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    const std::string key = "key";

    EXPECT_EQ(std::nullopt, client.Get(key));

    const std::string value = "value";
    client.Put(key, value);
    EXPECT_EQ(value, client.Get(key));

    client.Delete(key);
    EXPECT_EQ(std::nullopt, client.Get(key));
}

UTEST(Snapshot, GetMissingKey) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    auto snap = client.CreateSnapshot();
    EXPECT_EQ(std::nullopt, snap.Get("absent"));
}

UTEST(Snapshot, GetExistingKey) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("k", "v");
    auto snap = client.CreateSnapshot();
    EXPECT_EQ("v", snap.Get("k"));
}

UTEST(Snapshot, IsolatesFromLaterWrite) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("k", "old");
    auto snap = client.CreateSnapshot();
    client.Put("k", "new");

    EXPECT_EQ("old", snap.Get("k"));
    EXPECT_EQ("new", client.Get("k"));
}

UTEST(Snapshot, GetMany) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("a", "1");
    client.Put("c", "3");
    auto snap = client.CreateSnapshot();

    const std::vector<std::string_view> keys = {"a", "b", "c"};
    const auto result = snap.GetMany(keys);

    ASSERT_EQ(3, result.size());
    EXPECT_EQ("1", result[0]);
    EXPECT_EQ(std::nullopt, result[1]);
    EXPECT_EQ("3", result[2]);
}

UTEST(Snapshot, SurvivesClientDestruction) {
    const auto dir = TempDirectory::Create();

    Snapshot snap = [&] {
        storages::rocks::Client client{dir.GetPath()};
        client.Put("k", "v");
        return client.CreateSnapshot();
    }();

    EXPECT_EQ("v", snap.Get("k"));
}

UTEST(Rocks, NoTransactionThrows) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    EXPECT_THROW(client.BeginTransaction(), Exception);
}

}  // namespace

USERVER_NAMESPACE_END

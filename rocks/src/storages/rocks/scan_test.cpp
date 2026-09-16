#include <userver/storages/rocks/client.hpp>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/storages/rocks/cursor.hpp>
#include <userver/storages/rocks/key_value.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using fs::blocking::TempDirectory;
using storages::rocks::Cursor;
using storages::rocks::KeyValue;

std::vector<KeyValue> DrainCursor(Cursor& cursor) {
    std::vector<KeyValue> result;
    while (true) {
        auto batch = cursor.FetchBatch(100);
        if (batch.empty()) {
            break;
        }
        for (auto& kv : batch) {
            result.push_back(std::move(kv));
        }
    }
    return result;
}

UTEST(Scan, EmptyDb) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    auto cursor = client.Scan();
    EXPECT_TRUE(cursor.FetchBatch().empty());
}

UTEST(Scan, AllEntries) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("a", "1");
    client.Put("b", "2");
    client.Put("c", "3");

    auto cursor = client.Scan();
    const auto result = DrainCursor(cursor);

    ASSERT_EQ(3, result.size());
    EXPECT_EQ("a", result[0].key);
    EXPECT_EQ("1", result[0].value);
    EXPECT_EQ("b", result[1].key);
    EXPECT_EQ("2", result[1].value);
    EXPECT_EQ("c", result[2].key);
    EXPECT_EQ("3", result[2].value);
}

UTEST(Scan, PrefixFilter) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("a/1", "v1");
    client.Put("b/1", "v2");
    client.Put("a/2", "v3");

    auto cursor = client.Scan("a/");
    const auto result = DrainCursor(cursor);

    ASSERT_EQ(2, result.size());
    EXPECT_EQ("a/1", result[0].key);
    EXPECT_EQ("a/2", result[1].key);
}

UTEST(Scan, PrefixNoMatch) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("a", "1");
    client.Put("b", "2");

    auto cursor = client.Scan("z/");
    EXPECT_TRUE(cursor.FetchBatch().empty());
}

UTEST(Scan, Batching) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("1", "a");
    client.Put("2", "b");
    client.Put("3", "c");
    client.Put("4", "d");
    client.Put("5", "e");

    auto cursor = client.Scan();

    const auto batch1 = cursor.FetchBatch(2);
    ASSERT_EQ(2, batch1.size());
    EXPECT_EQ("1", batch1[0].key);
    EXPECT_EQ("2", batch1[1].key);

    const auto batch2 = cursor.FetchBatch(2);
    ASSERT_EQ(2, batch2.size());
    EXPECT_EQ("3", batch2[0].key);
    EXPECT_EQ("4", batch2[1].key);

    const auto batch3 = cursor.FetchBatch(2);
    ASSERT_EQ(1, batch3.size());
    EXPECT_EQ("5", batch3[0].key);

    EXPECT_TRUE(cursor.FetchBatch(2).empty());
}

UTEST(Scan, ResumeAcrossBatches) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("a", "1");
    client.Put("b", "2");
    client.Put("c", "3");

    auto cursor_full = client.Scan();
    const auto full = DrainCursor(cursor_full);

    auto cursor_one = client.Scan();
    std::vector<KeyValue> by_one;
    while (true) {
        auto batch = cursor_one.FetchBatch(1);
        if (batch.empty()) {
            break;
        }
        by_one.push_back(std::move(batch[0]));
    }

    ASSERT_EQ(full.size(), by_one.size());
    for (std::size_t i = 0; i < full.size(); ++i) {
        EXPECT_EQ(full[i].key, by_one[i].key);
        EXPECT_EQ(full[i].value, by_one[i].value);
    }
}

UTEST(SnapshotScan, IsolatesFromLaterWrite) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("a", "1");
    client.Put("b", "2");

    auto cursor = client.Scan();

    // Write after Scan() — must not appear in cursor results
    client.Put("c", "3");

    const auto result = DrainCursor(cursor);

    ASSERT_EQ(2, result.size());
    EXPECT_EQ("a", result[0].key);
    EXPECT_EQ("b", result[1].key);
}

}  // namespace

USERVER_NAMESPACE_END

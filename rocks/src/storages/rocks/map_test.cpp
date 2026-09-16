#include <userver/storages/rocks/client.hpp>

#include <userver/formats/parse/to.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/storages/rocks/db_builder.hpp>
#include <userver/storages/rocks/map.hpp>
#include <userver/storages/rocks/raw_map.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Custom types so ADL finds the overloads below (std::string has no associated
// namespace where we can inject ToString/Parse).
struct TestKey {
    std::string s;
};
struct TestValue {
    std::string s;
};

std::string ToString(const TestKey& k) { return k.s; }
std::string ToString(const TestValue& v) { return v.s; }

TestValue Parse(std::string_view raw, formats::parse::To<TestValue>) { return TestValue{std::string{raw}}; }

using fs::blocking::TempDirectory;
using storages::rocks::RawMap;
using storages::rocks::TransactionType;
using TestMap = storages::rocks::Map<TestKey, TestValue>;

UTEST(RawMap, LookupPresentKey) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("k", "v");

    auto raw = RawMap::FromSnapshot(client.CreateSnapshot());
    EXPECT_EQ("v", raw["k"]);
}

UTEST(RawMap, LookupAbsentKey) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    auto raw = RawMap::FromSnapshot(client.CreateSnapshot());
    EXPECT_FALSE(raw["missing"].has_value());
}

UTEST(RawMap, IsolatesFromLaterWrite) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("a", "1");
    auto raw = RawMap::FromSnapshot(client.CreateSnapshot());

    client.Put("b", "2");

    EXPECT_TRUE(raw["a"].has_value());
    EXPECT_FALSE(raw["b"].has_value());
}

UTEST(Map, LookupPresentKey) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    client.Put("hello", "world");

    auto map = TestMap::FromSnapshot(client.CreateSnapshot());
    const auto result = map[TestKey{"hello"}];

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("world", result->s);
}

UTEST(Map, LookupAbsentKey) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath()};

    auto map = TestMap::FromSnapshot(client.CreateSnapshot());
    EXPECT_FALSE(map[TestKey{"missing"}].has_value());
}

UTEST(DbBuilder, CommitPersists) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath(), TransactionType::kPessimistic};

    storages::rocks::DbBuilder<TestKey, TestValue> builder{client};
    builder.Emplace(TestKey{"x"}, TestValue{"1"});
    builder.Emplace(TestKey{"y"}, TestValue{"2"});
    builder.Commit();

    auto map = TestMap::FromSnapshot(client.CreateSnapshot());
    const auto x = map[TestKey{"x"}];
    const auto y = map[TestKey{"y"}];

    ASSERT_TRUE(x.has_value());
    EXPECT_EQ("1", x->s);
    ASSERT_TRUE(y.has_value());
    EXPECT_EQ("2", y->s);
}

UTEST(DbBuilder, RemovesOtherKeys) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath(), TransactionType::kPessimistic};

    client.Put("old", "gone");

    storages::rocks::DbBuilder<TestKey, TestValue> builder{client};
    builder.Emplace(TestKey{"new"}, TestValue{"kept"});
    builder.Commit();

    auto map = TestMap::FromSnapshot(client.CreateSnapshot());
    EXPECT_FALSE(map[TestKey{"old"}].has_value());
    ASSERT_TRUE(map[TestKey{"new"}].has_value());
    EXPECT_EQ("kept", map[TestKey{"new"}]->s);
}

UTEST(DbBuilder, RollbackOnDestruct) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath(), TransactionType::kPessimistic};

    client.Put("existing", "value");

    {
        storages::rocks::DbBuilder<TestKey, TestValue> builder{client};
        builder.Emplace(TestKey{"new"}, TestValue{"data"});
        // Commit() NOT called — destructor rolls back
    }

    const auto existing = client.Get("existing");
    const auto new_key = client.Get("new");

    ASSERT_TRUE(existing.has_value());
    EXPECT_EQ("value", *existing);
    EXPECT_FALSE(new_key.has_value());
}

UTEST(DbBuilder, DuplicateEmplace) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{dir.GetPath(), TransactionType::kPessimistic};

    storages::rocks::DbBuilder<TestKey, TestValue> builder{client};
    builder.Emplace(TestKey{"k"}, TestValue{"first"});
    builder.Emplace(TestKey{"k"}, TestValue{"last"});
    builder.Commit();

    const auto result = client.Get("k");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("last", *result);
}

}  // namespace

USERVER_NAMESPACE_END

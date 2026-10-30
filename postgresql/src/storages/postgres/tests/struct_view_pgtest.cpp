#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/composite_types.hpp>
#include <userver/storages/postgres/io/uuid.hpp>

#include <userver/storages/postgres/io/struct_view.hpp>
#include <userver/storages/postgres/transaction.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {
constexpr const char* const kSchemaName = "__pgtest";
const std::string kCreateTestSchema = "create schema if not exists __pgtest";
const std::string kDropTestSchema = "drop schema if exists __pgtest cascade";
const boost::uuids::uuid kNilUuid = boost::uuids::string_generator()("00000000-0000-0000-0000-000000000000");
const std::string kSelectFullUserInput = "select $1::uuid, $2::text, $3::text";

struct User {
    boost::uuids::uuid id;
    std::string name;
    std::string email;
};

using FullView = io::StructView<User>;
using FullViewExplicit = io::StructView<User, &User::id, &User::name, &User::email>;
using MessedNameEmailView = io::StructView<User, &User::id, &User::email, &User::name>;
using NoEmailView = io::StructView<User, &User::id, &User::name>;

using NoEmailVector = std::vector<NoEmailView>;
static_assert(FullView::size == 3);
static_assert(FullViewExplicit::size == 3);
static_assert(MessedNameEmailView::size == 3);
static_assert(NoEmailView::size == 2);
static_assert(io::traits::IsStructView<NoEmailVector::value_type>::value);
static_assert(std::is_same_v<meta::RebindContainer<NoEmailVector, User>::value_type, User>);
static_assert(std::is_same_v<meta::RebindContainer<NoEmailVector, User>, std::vector<User>>);

static_assert(
    std::is_same_v<decltype(std::declval<pg::Row>().As<NoEmailView>(pg::kRowTag)), User>,
    "Expect to return user"
);
static_assert(
    std::is_same_v<decltype(std::declval<pg::ResultSet>().AsSetOf<NoEmailView>(pg::kRowTag))::value_type, User>,
    "Expect to return user"
);
static_assert(
    std::is_same_v<
        decltype(std::declval<pg::ResultSet>().AsContainer<std::vector<NoEmailView>>(pg::kRowTag))::value_type,
        User>,
    "Expect to return container of underlying values"
);
static_assert(
    std::is_same_v<
        decltype(std::declval<pg::ResultSet>().AsContainer<std::vector<NoEmailView>>(pg::kRowTag)),
        std::vector<User>>,
    "Expect to return container of underlying values"
);

struct Item {
    int id{};
    std::string name;
    std::string description;

    bool operator==(const Item&) const = default;
};

class IntrusiveItem {
public:
    IntrusiveItem() = default;
    IntrusiveItem(int id, std::string name) : id_{id}, name_{std::move(name)} {}

    auto Introspect() { return std::tie(id_, name_); }

    bool operator==(const IntrusiveItem&) const = default;

private:
    int id_{};
    std::string name_;
};

struct NonCopyableUser {
    int id{};
    std::unique_ptr<int> handle;
};

struct DerivedUser : User {
    int extra{};
};

struct ConvertibleToUser {
    operator User() const { return {}; }
};

using NonCopyableIdView = io::StructView<NonCopyableUser, &NonCopyableUser::id>;
static_assert(NonCopyableIdView::size == 1);
static_assert(std::is_same_v<
              decltype(NonCopyableIdView::Tie(std::declval<const NonCopyableUser&>())),
              std::tuple<const int&>>);

template <typename View, typename Value>
concept CanTie = requires(Value& value) { View::Tie(value); };

template <typename View, typename Value>
concept CanTieTemporary = requires(Value&& value) { View::Tie(std::move(value)); };

static_assert(CanTie<NoEmailView, const DerivedUser>);
static_assert(std::is_same_v<
              decltype(FullView::Tie(std::declval<const DerivedUser&>())),
              decltype(FullView::Tie(std::declval<const User&>()))>);
static_assert(!CanTie<NoEmailView, const ConvertibleToUser>);
static_assert(!CanTieTemporary<NoEmailView, User>);

using ItemIdNameView = io::StructView<Item, &Item::id, &Item::name>;
using ItemNameIdView = io::StructView<Item, &Item::name, &Item::id>;

constexpr int kItemsCount = 1001;
constexpr std::size_t kChunkRows = 100;
const std::string kUnnestTwoColumns = "select * from unnest($1, $2)";
const std::string kUnnestThreeColumns = "select * from unnest($1, $2, $3)";

static_assert(std::is_same_v<
              decltype(io::MakeViewContainer<&Item::id, &Item::name>(std::declval<const std::vector<Item>&>())),
              io::detail::ViewContainer<ItemIdNameView, std::vector<Item>>>);
static_assert(std::is_same_v<
              decltype(io::StructViewContainer<ItemNameIdView>(std::declval<const std::vector<Item>&>())),
              io::detail::ViewContainer<ItemNameIdView, std::vector<Item>>>);
static_assert(tt::IsViewContainer<io::detail::ViewContainer<ItemIdNameView, std::vector<Item>>>::value);
static_assert(!tt::IsViewContainer<std::vector<Item>>::value);

std::vector<Item> MakeItems() {
    std::vector<Item> items;
    items.reserve(kItemsCount);
    for (int i = 0; i < kItemsCount; ++i) {
        items.push_back(Item{i, "name" + std::to_string(i), "description" + std::to_string(i)});
    }
    return items;
}

std::vector<Item> WithoutDescription(std::vector<Item> items) {
    for (auto& item : items) {
        item.description.clear();
    }
    return items;
}

std::vector<IntrusiveItem> MakeIntrusiveItems() {
    std::vector<IntrusiveItem> items;
    items.reserve(kItemsCount);
    for (int i = 0; i < kItemsCount; ++i) {
        items.emplace_back(i, "name" + std::to_string(i));
    }
    return items;
}

}  // namespace

UTEST_P(PostgreConnection, StructViewRead) {
    CheckConnection(GetConn());
    ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";

    pg::ResultSet res{nullptr};
    UASSERT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
    UASSERT_NO_THROW(GetConn()->Execute(kCreateTestSchema)) << "Create schema";

    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select '00000000-0000-0000-0000-000000000000'::uuid, 'foo', 'foo@bar.baz'")
    );

    ASSERT_FALSE(res.IsEmpty());

    {
        auto user = res.Front().As<FullView>(pg::kRowTag);
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.name, "foo");
        EXPECT_EQ(user.email, "foo@bar.baz");
    }
    {
        auto user = res.AsSingleRow<FullView>(pg::kRowTag);
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.name, "foo");
        EXPECT_EQ(user.email, "foo@bar.baz");
    }
    {
        auto user = res.Front().As<FullViewExplicit>(pg::kRowTag);
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.name, "foo");
        EXPECT_EQ(user.email, "foo@bar.baz");
    }
    {
        auto user = res.AsSingleRow<FullViewExplicit>(pg::kRowTag);
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.name, "foo");
        EXPECT_EQ(user.email, "foo@bar.baz");
    }
    {
        auto user = res.Front().As<MessedNameEmailView>(pg::kRowTag);
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.email, "foo");
        EXPECT_EQ(user.name, "foo@bar.baz");
    }
    {
        auto user = res.AsSingleRow<MessedNameEmailView>(pg::kRowTag);
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.email, "foo");
        EXPECT_EQ(user.name, "foo@bar.baz");
    }
    {
        auto set = res.AsSetOf<FullView>(pg::kRowTag);
        EXPECT_EQ(set.Size(), 1);
        auto user = *set.begin();
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.name, "foo");
        EXPECT_EQ(user.email, "foo@bar.baz");
    }
    {
        auto vec = res.AsContainer<std::vector<FullView>>(pg::kRowTag);
        EXPECT_EQ(vec.size(), 1);
        auto user = vec.front();
        EXPECT_EQ(user.id, kNilUuid);
        EXPECT_EQ(user.name, "foo");
        EXPECT_EQ(user.email, "foo@bar.baz");
    }
}

UTEST_P(PostgreConnection, StructViewWrite) {
    CheckConnection(GetConn());
    ASSERT_FALSE(GetConn()->IsReadOnly()) << "Expect a read-write connection";

    pg::ResultSet res{nullptr};
    UASSERT_NO_THROW(GetConn()->Execute(kDropTestSchema)) << "Drop schema";
    UASSERT_NO_THROW(GetConn()->Execute(kCreateTestSchema)) << "Create schema";

    User user{.id = kNilUuid, .name = "foo", .email = "foo@bar.baz"};
    {
        UASSERT_NO_THROW(res = GetConn()->Execute(kSelectFullUserInput, io::StructView<User>{user}));
        auto u = res.AsSingleRow<User>(pg::kRowTag);
        EXPECT_EQ(u.id, user.id);
        EXPECT_EQ(u.name, user.name);
        EXPECT_EQ(u.email, user.email);
    }
    {
        // MakeStructView deduces the viewed class from its argument, so the members
        // are the only thing written out. Nothing instantiated it before this.
        UASSERT_NO_THROW(
            res = GetConn()->Execute(
                kSelectFullUserInput, io::MakeStructView<&User::id, &User::email, &User::name>(user)
            )
        );
        auto u = res.AsSingleRow<User>(pg::kRowTag);
        EXPECT_EQ(u.id, user.id);
        EXPECT_EQ(u.name, user.email);
        EXPECT_EQ(u.email, user.name);
    }
    {
        UASSERT_NO_THROW(res = GetConn()->Execute(kSelectFullUserInput, FullView{user}));
        auto u = res.AsSingleRow<User>(pg::kRowTag);
        EXPECT_EQ(u.id, user.id);
        EXPECT_EQ(u.name, user.name);
        EXPECT_EQ(u.email, user.email);
    }
    {
        UASSERT_NO_THROW(res = GetConn()->Execute(kSelectFullUserInput, FullViewExplicit{user}));
        auto u = res.AsSingleRow<User>(pg::kRowTag);
        EXPECT_EQ(u.id, user.id);
        EXPECT_EQ(u.name, user.name);
        EXPECT_EQ(u.email, user.email);
    }
    {
        // The pack is the order, not the struct's layout: name and email go out
        // swapped, so they come back swapped.
        UASSERT_NO_THROW(res = GetConn()->Execute(kSelectFullUserInput, MessedNameEmailView{user}));
        auto u = res.AsSingleRow<User>(pg::kRowTag);
        EXPECT_EQ(u.id, user.id);
        EXPECT_EQ(u.name, user.email);
        EXPECT_EQ(u.email, user.name);
    }
    {
        UASSERT_NO_THROW(res = GetConn()->Execute("select $1::uuid, $2::text", NoEmailView{user}));
        auto u = res.AsSingleRow<NoEmailView>(pg::kRowTag);
        EXPECT_EQ(u.id, user.id);
        EXPECT_EQ(u.name, user.name);
        EXPECT_EQ(u.email, "");
    }
}

UTEST_P(PostgreConnection, StructViewDecomposeSubset) {
    CheckConnection(GetConn());
    const auto items = MakeItems();

    pg::Transaction trx{std::move(GetConn())};
    const auto res = trx.ExecuteDecompose(kUnnestTwoColumns, io::MakeViewContainer<&Item::id, &Item::name>(items));

    EXPECT_EQ(WithoutDescription(items), res.AsContainer<std::vector<ItemIdNameView>>(pg::kRowTag));
}

UTEST_P(PostgreConnection, StructViewDecomposeReordered) {
    CheckConnection(GetConn());
    const auto items = MakeItems();

    pg::Transaction trx{std::move(GetConn())};
    const auto res = trx.ExecuteDecompose(
        pg::OptionalCommandControl{},
        "select * from unnest($1::text[], $2::integer[])",
        io::StructViewContainer<ItemNameIdView>(items)
    );

    EXPECT_EQ(WithoutDescription(items), res.AsContainer<std::vector<ItemNameIdView>>(pg::kRowTag));
}

UTEST_P(PostgreConnection, StructViewDecomposeFull) {
    CheckConnection(GetConn());
    const auto items = MakeItems();

    pg::Transaction trx{std::move(GetConn())};
    const auto res = trx.ExecuteDecompose(kUnnestThreeColumns, io::MakeViewContainer<>(items));

    EXPECT_EQ(items, res.AsContainer<std::vector<Item>>(pg::kRowTag));
}

UTEST_P(PostgreConnection, StructViewDecomposeIntrusive) {
    CheckConnection(GetConn());
    const auto items = MakeIntrusiveItems();

    pg::Transaction trx{std::move(GetConn())};
    const auto through_view = trx.ExecuteDecompose(kUnnestTwoColumns, io::MakeViewContainer<>(items));
    const auto plain = trx.ExecuteDecompose(kUnnestTwoColumns, items);

    EXPECT_EQ(items, through_view.AsContainer<std::vector<IntrusiveItem>>(pg::kRowTag));
    EXPECT_EQ(items, plain.AsContainer<std::vector<IntrusiveItem>>(pg::kRowTag));
}

UTEST_P(PostgreConnection, StructViewDecomposeBulk) {
    CheckConnection(GetConn());
    GetConn()->Execute("create temporary table struct_view_bulk(name text, id integer)");
    const auto items = MakeItems();

    pg::Transaction trx{std::move(GetConn())};
    UEXPECT_NO_THROW(trx.ExecuteDecomposeBulk(
        "insert into struct_view_bulk(name, id) select * from unnest($1, $2)",
        io::StructViewContainer<ItemNameIdView>(items),
        kChunkRows
    ));
    const auto res = trx.Execute("select name, id from struct_view_bulk order by id");

    EXPECT_EQ(WithoutDescription(items), res.AsContainer<std::vector<ItemNameIdView>>(pg::kRowTag));
}

UTEST_P(PostgreConnection, StructViewDecomposeBulkCommandControl) {
    CheckConnection(GetConn());
    GetConn()->Execute("create temporary table struct_view_bulk_cc(id integer, name text)");
    const auto items = MakeItems();

    pg::Transaction trx{std::move(GetConn())};
    UEXPECT_NO_THROW(trx.ExecuteDecomposeBulk(
        pg::OptionalCommandControl{},
        "insert into struct_view_bulk_cc(id, name) select * from unnest($1, $2)",
        io::MakeViewContainer<&Item::id, &Item::name>(items),
        kChunkRows
    ));
    const auto res = trx.Execute("select id, name from struct_view_bulk_cc order by id");

    EXPECT_EQ(WithoutDescription(items), res.AsContainer<std::vector<ItemIdNameView>>(pg::kRowTag));
}

USERVER_NAMESPACE_END

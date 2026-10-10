#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/composite_types.hpp>
#include <userver/storages/postgres/io/uuid.hpp>

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

USERVER_NAMESPACE_END
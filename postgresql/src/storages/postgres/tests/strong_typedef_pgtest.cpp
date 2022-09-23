#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/array_types.hpp>
#include <userver/storages/postgres/io/strong_typedef.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace static_test {

/*! [Strong typedef] */
using StringTypedef = utils::StrongTypedef<struct TestTypedef, std::string>;
// This strong typedef will be nullable
using OptStringTypedef =
    utils::StrongTypedef<struct TestTypedef, std::optional<std::string>>;
using IntTypedef = utils::StrongTypedef<struct TestTypedef, pg::Integer>;
/*! [Strong typedef] */

struct UserType {
  StringTypedef s;
  IntTypedef i;
};

using UserTypedef = utils::StrongTypedef<struct TestTypedef, UserType>;

enum class MappedEnum { kOne, kTwo };

enum class UnusableEnumTypedef : unsigned int {};

}  // namespace static_test

/*! [Enum typedef] */
namespace sample {
enum class EnumStrongTypedef : int {};
}  // namespace sample

template <>
struct storages::postgres::io::traits::CanUseEnumAsStrongTypedef<
    sample::EnumStrongTypedef> : std::true_type {};
/*! [Enum typedef] */

namespace storages::postgres::io {

template <>
struct CppToUserPg<static_test::UserType> {
  // dummy name, not actually used in test
  static constexpr DBTypeName postgres_name = "schema.name";
};

template <>
struct CppToUserPg<static_test::MappedEnum>
    : EnumMappingBase<static_test::MappedEnum> {
  // dummy name, not actually used in test
  static constexpr DBTypeName postgres_name = "schema.name";
  static constexpr Enumerator enumerators[]{{EnumType::kOne, "one"},
                                            {EnumType::kTwo, "two"}};
};

}  // namespace storages::postgres::io

namespace static_test {

// Strong typedef to system pg type
static_assert(tt::kIsMappedToPg<StringTypedef>,
              "Strong typedef must map to underlying type mapping");
static_assert(
    std::is_same<io::CppToPg<StringTypedef>::Mapping,
                 io::CppToSystemPg<std::string>>::value,
    "Strong typedef must have the same mapping as the underlying type");
static_assert(tt::kHasParser<StringTypedef>,
              "Strong typedef to a defined type must have a parser");
static_assert(tt::kHasFormatter<StringTypedef>,
              "Strong typedef to a defined type must have a formatter");
static_assert(!tt::kIsNullable<StringTypedef>,
              "Strong typedef must derive nullability from underlying type");

static_assert(tt::kIsMappedToPg<OptStringTypedef>,
              "Strong typedef must map to underlying type mapping");
static_assert(
    std::is_same<io::CppToPg<OptStringTypedef>::Mapping,
                 io::CppToSystemPg<std::string>>::value,
    "Strong typedef must have the same mapping as the underlying type");
static_assert(tt::kHasParser<OptStringTypedef>,
              "Strong typedef to a defined type must have a parser");
static_assert(tt::kHasFormatter<OptStringTypedef>,
              "Strong typedef to a defined type must have a formatter");
static_assert(tt::kIsNullable<OptStringTypedef>,
              "Strong typedef must derive nullability from underlying type");

static_assert(tt::kIsMappedToPg<IntTypedef>,
              "Strong typedef must map to underlying type mapping");
static_assert(
    std::is_same<io::CppToPg<IntTypedef>::Mapping,
                 io::CppToSystemPg<pg::Integer>>::value,
    "Strong typedef must have the same mapping as the underlying type");
static_assert(tt::kHasParser<IntTypedef>,
              "Strong typedef to a defined type must have a parser");
static_assert(tt::kHasFormatter<IntTypedef>,
              "Strong typedef to a defined type must have a formatter");
static_assert(!tt::kIsNullable<IntTypedef>,
              "Strong typedef must derive nullability from underlying type");

static_assert(tt::kIsMappedToPg<UserTypedef>,
              "Strong typedef must map to underlying type mapping");
static_assert(
    std::is_same<io::CppToPg<UserTypedef>::Mapping,
                 io::CppToUserPg<UserType>>::value,
    "Strong typedef must have the same mapping as the underlying type");
static_assert(tt::kHasParser<UserTypedef>,
              "Strong typedef to a defined type must have a parser");
static_assert(tt::kHasFormatter<UserTypedef>,
              "Strong typedef to a defined type must have a formatter");
static_assert(!tt::kIsNullable<UserTypedef>,
              "Strong typedef must derive nullability from underlying type");

// Check mapping calculation doesn't break hand-mapped types
static_assert(tt::kIsMappedToPg<pg::TimePointTz>);
static_assert(std::is_same<io::CppToPg<pg::TimePointTz>::Mapping,
                           io::CppToSystemPg<pg::TimePointTz>>::value);
static_assert(std::is_same<tt::IO<pg::TimePointTz>::ParserType,
                           io::BufferParser<pg::TimePointTz>>::value);
static_assert(std::is_same<tt::IO<pg::TimePointTz>::FormatterType,
                           io::BufferFormatter<pg::TimePointTz>>::value);

static_assert(!tt::CanUseEnumAsStrongTypedef<std::string>(), "not an enum");
static_assert(!tt::CanUseEnumAsStrongTypedef<int>(), "not an enum");

// enum with unsigned underlying type cannot be used as strong typedef with
// postgres
static_assert(
    !tt::CanUseEnumAsStrongTypedef<UnusableEnumTypedef>(),
    "Enumeration with unsigned underlying type cannot be used with postgres");

static_assert(!tt::CanUseEnumAsStrongTypedef<MappedEnum>(),
              "Mapped enum cannot be used as a strong typedef");

static_assert(tt::CanUseEnumAsStrongTypedef<sample::EnumStrongTypedef>(),
              "Enum with signed underlying type and no mapping can be used as "
              "a strong typedef");
static_assert(tt::kIsMappedToPg<sample::EnumStrongTypedef>,
              "A valid enum strong typedef must have a parser");
static_assert(tt::kHasParser<sample::EnumStrongTypedef>,
              "A valid enum strong typedef must have a parser");
static_assert(tt::kHasFormatter<sample::EnumStrongTypedef>,
              "A valid enum strong typedef must have a formatter");
static_assert(!tt::kIsNullable<sample::EnumStrongTypedef>,
              "Strong typedef must derive nullability from underlying type");

}  // namespace static_test

namespace {

UTEST_P(PostgreConnection, StringStrongTypedef) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  static_test::StringTypedef str{"test"};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", str));
  EXPECT_EQ(str, res[0][0].As<static_test::StringTypedef>());
  // Row interface
  EXPECT_EQ(str, res[0].As<static_test::StringTypedef>());
  // Single row interface
  EXPECT_EQ(str, res.AsSingleRow<static_test::StringTypedef>());
  // As container interface
  EXPECT_EQ(str, res.AsContainer<std::vector<static_test::StringTypedef>>()[0]);
}

UTEST_P(PostgreConnection, StringStrongTypedefArray) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  using StringTypedefVector = std::vector<static_test::StringTypedef>;

  StringTypedefVector str_vec{static_test::StringTypedef{"test"},
                              static_test::StringTypedef{}};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", str_vec));
  EXPECT_EQ(str_vec, res[0][0].As<StringTypedefVector>());
  // Row interface
  EXPECT_EQ(str_vec, res[0].As<StringTypedefVector>());
  // Single row interface
  EXPECT_EQ(str_vec, res.AsSingleRow<StringTypedefVector>());
  // As container interface
  EXPECT_EQ(str_vec, res.AsContainer<std::vector<StringTypedefVector>>()[0]);
}

UTEST_P(PostgreConnection, IntStrongTypedef) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  static_test::IntTypedef i{42};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", i));
  EXPECT_EQ(i, res[0][0].As<static_test::IntTypedef>());
  // Row interface
  EXPECT_EQ(i, res[0].As<static_test::IntTypedef>());
  // Single row interface
  EXPECT_EQ(i, res.AsSingleRow<static_test::IntTypedef>());
  // As container interface
  EXPECT_EQ(i, res.AsContainer<std::vector<static_test::IntTypedef>>()[0]);
}

UTEST_P(PostgreConnection, IntStrongTypedefArray) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  using IntTypedefVector = std::vector<static_test::IntTypedef>;

  IntTypedefVector i_vec{static_test::IntTypedef{42},
                         static_test::IntTypedef{13}};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", i_vec));
  EXPECT_EQ(i_vec, res[0][0].As<IntTypedefVector>());
  // Row interface
  EXPECT_EQ(i_vec, res[0].As<IntTypedefVector>());
  // Single row interface
  EXPECT_EQ(i_vec, res.AsSingleRow<IntTypedefVector>());
  // As container interface
  EXPECT_EQ(i_vec, res.AsContainer<std::vector<IntTypedefVector>>()[0]);
}

UTEST_P(PostgreConnection, IntEnumStrongTypedef) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  sample::EnumStrongTypedef i{42};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", i));
  EXPECT_EQ(i, res[0][0].As<sample::EnumStrongTypedef>());
  // Row interface
  EXPECT_EQ(i, res[0].As<sample::EnumStrongTypedef>());
  // Single row interface
  EXPECT_EQ(i, res.AsSingleRow<sample::EnumStrongTypedef>());
  // As container interface
  EXPECT_EQ(i, res.AsContainer<std::vector<sample::EnumStrongTypedef>>()[0]);
}

UTEST_P(PostgreConnection, IntEnumStrongTypedefArray) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  using EnumTypedefVector = std::vector<sample::EnumStrongTypedef>;

  EnumTypedefVector i_vec{sample::EnumStrongTypedef{42},
                          sample::EnumStrongTypedef{13}};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", i_vec));
  EXPECT_EQ(i_vec, res[0][0].As<EnumTypedefVector>());
  // Row interface
  EXPECT_EQ(i_vec, res[0].As<EnumTypedefVector>());
  // Single row interface
  EXPECT_EQ(i_vec, res.AsSingleRow<EnumTypedefVector>());
  // As container interface
  EXPECT_EQ(i_vec, res.AsContainer<std::vector<EnumTypedefVector>>()[0]);
}

}  // namespace

USERVER_NAMESPACE_END

#include <gtest/gtest.h>

#include <userver/storages/postgres/detail/query_parameters.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

using namespace io::traits;

struct __no_output_operator {};
static_assert(!HasOutputOperator<__no_output_operator>::value,
              "Test output metafunction");
static_assert(HasOutputOperator<int>::value, "Test output metafunction");
static_assert(!kHasFormatter<__no_output_operator>,
              "Test has formatter metafuction");

static_assert(kHasFormatter<std::optional<int>>,
              "Test has formatter metafuction");

}  // namespace static_test

namespace {

const pg::UserTypes types;

TEST(PostgreIO, OutputIntegral) {
  pg::detail::QueryParameters params;
  pg::Smallint s{42};
  pg::Integer i{42};
  pg::Bigint b{42};

  params.Write(types, s);
  params.Write(types, i);
  params.Write(types, b);

  EXPECT_EQ(3, params.Size());

  params.Write(types, s, i, b);
  EXPECT_EQ(6, params.Size());
}

TEST(PostgreIO, OutputString) {
  pg::detail::QueryParameters params;
  const char* c_str = "foo";
  std::string str{"foo"};
  std::string_view sw{str};

  params.Write(types, "foo");
  params.Write(types, c_str);
  params.Write(types, str);
  params.Write(types, sw);

  EXPECT_EQ(4, params.Size());

  EXPECT_EQ(1, params.ParamFormatsBuffer()[0]) << "Binary format";
  EXPECT_EQ(3, params.ParamLengthsBuffer()[0]) << "No zero terminator";

  EXPECT_EQ(1, params.ParamFormatsBuffer()[1]) << "Binary format";
  EXPECT_EQ(3, params.ParamLengthsBuffer()[1]) << "No zero terminator";

  EXPECT_EQ(1, params.ParamFormatsBuffer()[2]) << "Binary format";
  EXPECT_EQ(3, params.ParamLengthsBuffer()[2]) << "No zero terminator";

  EXPECT_EQ(1, params.ParamFormatsBuffer()[3]) << "Binary format";
  EXPECT_EQ(3, params.ParamLengthsBuffer()[3]) << "No zero terminator";
}

TEST(PostgreIO, OutputFloat) {
  pg::detail::QueryParameters params;
  EXPECT_NO_THROW(params.Write(types, 3.14f));
  EXPECT_EQ(1, params.Size());
  EXPECT_EQ(static_cast<pg::Oid>(pg::io::PredefinedOids::kFloat4),
            params.ParamTypesBuffer()[0]);
}

}  // namespace

USERVER_NAMESPACE_END

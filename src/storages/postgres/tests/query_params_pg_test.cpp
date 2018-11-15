#include <gtest/gtest.h>

#include <storages/postgres/detail/query_parameters.hpp>
#include <storages/postgres/io/force_text.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

using namespace io::traits;

struct __no_output_operator {};
static_assert(!detail::HasOutputOperator<__no_output_operator>::value,
              "Test output metafunction");
static_assert(detail::HasOutputOperator<int>::value,
              "Test output metafunction");
static_assert(!kHasTextFormatter<__no_output_operator>,
              "Test has formatter metafuction");

// static_assert(
//    (pg::io::traits::HasFormatter<boost::optional<int>,
//                                  pg::io::DataFormat::kTextDataFormat>::value
//                                  ==
//     true),
//    "Test has formatter metafuction");

}  // namespace static_test

TEST(PostgreIO, OutputIntegral) {
  pg::detail::QueryParameters params;
  pg::Smallint s{42};
  pg::Integer i{42};
  pg::Bigint b{42};

  params.Write(s);
  params.Write(i);
  params.Write(b);

  EXPECT_EQ(3, params.Size());

  params.Write(s, i, b);
  EXPECT_EQ(6, params.Size());
}

TEST(PostgreIO, OutputString) {
  pg::detail::QueryParameters params;
  const char* c_str = "foo";
  std::string str{"foo"};

  params.Write("foo");
  params.Write(pg::ForceTextFormat("foo"));
  params.Write(c_str);
  params.Write(str);

  EXPECT_EQ(4, params.Size());

  EXPECT_EQ(1, params.ParamFormatsBuffer()[0]) << "Binary format";
  EXPECT_EQ(3, params.ParamLengthsBuffer()[0]) << "No zero terminator";

  EXPECT_EQ(0, params.ParamFormatsBuffer()[1]) << "Text format";
  EXPECT_EQ(4, params.ParamLengthsBuffer()[1]) << "Zero terminator";

  EXPECT_EQ(1, params.ParamFormatsBuffer()[2]) << "Binary format";
  EXPECT_EQ(3, params.ParamLengthsBuffer()[2]) << "No zero terminator";

  EXPECT_EQ(1, params.ParamFormatsBuffer()[3]) << "Binary format";
  EXPECT_EQ(3, params.ParamLengthsBuffer()[3]) << "No zero terminator";
}

TEST(PostgreIO, OutputFloat) {
  pg::detail::QueryParameters params;
  EXPECT_NO_THROW(params.Write(3.14f));
  EXPECT_EQ(1, params.Size());
  EXPECT_EQ(static_cast<pg::Oid>(pg::io::PredefinedOids::kFloat4),
            params.ParamTypesBuffer()[0]);
}

#include <gtest/gtest.h>

#include <storages/postgres/detail/query_parameters.hpp>

namespace pg = storages::postgres;

namespace static_test {

struct __no_output_operator {};
static_assert(
    pg::io::traits::detail::HasOutputOperator<__no_output_operator>::value ==
        false,
    "Test output metafunction");
static_assert(pg::io::traits::detail::HasOutputOperator<int>::value == true,
              "Test output metafunction");
static_assert(
    (pg::io::traits::HasFormatter<__no_output_operator,
                                  pg::io::DataFormat::kTextDataFormat>::value ==
     false),
    "Test has formatter metafuction");
static_assert((pg::io::traits::HasFormatter<
                   int, pg::io::DataFormat::kTextDataFormat>::value == true),
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
  params.Write(c_str);
  params.Write(str);

  EXPECT_EQ(3, params.Size());
}

TEST(PostgreIO, OutputFloat) {
  pg::detail::QueryParameters params;
  EXPECT_NO_THROW(params.Write(3.14f));
  EXPECT_EQ(1, params.Size());
  EXPECT_EQ(static_cast<pg::Oid>(pg::io::PredefinedOids::kFloat4),
            params.ParamTypesBuffer()[0]);
}

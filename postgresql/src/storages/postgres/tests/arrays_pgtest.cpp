#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/array_types.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace static_test {

using one_dim_vector = std::vector<int>;
using two_dim_vector = std::vector<one_dim_vector>;
using three_dim_vector = std::vector<two_dim_vector>;

constexpr std::size_t kDimOne = 3;
constexpr std::size_t kDimTwo = 2;
constexpr std::size_t kDimThree = 1;

using one_dim_array = std::array<int, kDimOne>;
using two_dim_array = std::array<one_dim_array, kDimTwo>;
using three_dim_array = std::array<two_dim_array, kDimThree>;

using vector_of_arrays = std::vector<two_dim_array>;

using unordered_set = std::unordered_set<int>;
using vector_of_unordered_sets = std::vector<unordered_set>;
using array_of_vectors_of_unordered_sets =
    std::array<vector_of_unordered_sets, kDimOne>;

using one_dim_set = std::set<int>;
using two_dim_set = std::set<one_dim_set>;
using set_of_arrays = std::set<one_dim_array>;
using set_of_vectors = std::set<one_dim_vector>;

static_assert(!tt::kIsCompatibleContainer<int>);

static_assert(tt::kIsCompatibleContainer<one_dim_vector>);
static_assert(tt::kIsCompatibleContainer<two_dim_vector>);
static_assert(tt::kIsCompatibleContainer<three_dim_vector>);

static_assert(tt::kIsCompatibleContainer<one_dim_array>);
static_assert(tt::kIsCompatibleContainer<two_dim_array>);
static_assert(tt::kIsCompatibleContainer<three_dim_array>);

static_assert(tt::kIsCompatibleContainer<vector_of_arrays>);

static_assert(tt::kIsCompatibleContainer<unordered_set>);
static_assert(tt::kIsCompatibleContainer<vector_of_unordered_sets>);
static_assert(tt::kIsCompatibleContainer<array_of_vectors_of_unordered_sets>);

static_assert(tt::kIsCompatibleContainer<one_dim_set>);
static_assert(tt::kIsCompatibleContainer<two_dim_set>);
static_assert(tt::kIsCompatibleContainer<set_of_arrays>);
static_assert(tt::kIsCompatibleContainer<set_of_vectors>);

static_assert(tt::kDimensionCount<one_dim_vector> == 1);
static_assert(tt::kDimensionCount<two_dim_vector> == 2);
static_assert(tt::kDimensionCount<three_dim_vector> == 3);

static_assert(tt::kDimensionCount<one_dim_array> == 1);
static_assert(tt::kDimensionCount<two_dim_array> == 2);
static_assert(tt::kDimensionCount<three_dim_array> == 3);

static_assert(tt::kDimensionCount<vector_of_arrays> == 3);

static_assert(tt::kDimensionCount<unordered_set> == 1);
static_assert(tt::kDimensionCount<vector_of_unordered_sets> == 2);
static_assert(tt::kDimensionCount<array_of_vectors_of_unordered_sets> == 3);

static_assert(tt::kDimensionCount<one_dim_set> == 1);
static_assert(tt::kDimensionCount<two_dim_set> == 2);
static_assert(tt::kDimensionCount<set_of_arrays> == 2);
static_assert(tt::kDimensionCount<set_of_vectors> == 2);

static_assert((
    std::is_same<tt::ContainerFinalElement<one_dim_vector>::type, int>::value));
static_assert((
    std::is_same<tt::ContainerFinalElement<two_dim_vector>::type, int>::value));
static_assert(std::is_same<tt::ContainerFinalElement<three_dim_vector>::type,
                           int>::value);

static_assert(
    (std::is_same<tt::ContainerFinalElement<one_dim_array>::type, int>::value));
static_assert(
    (std::is_same<tt::ContainerFinalElement<two_dim_array>::type, int>::value));
static_assert(
    std::is_same<tt::ContainerFinalElement<three_dim_array>::type, int>::value);

static_assert(std::is_same<tt::ContainerFinalElement<vector_of_arrays>::type,
                           int>::value);

static_assert(
    std::is_same<tt::ContainerFinalElement<unordered_set>::type, int>::value);
static_assert(
    std::is_same<tt::ContainerFinalElement<vector_of_unordered_sets>::type,
                 int>::value);
static_assert(
    std::is_same<
        tt::ContainerFinalElement<array_of_vectors_of_unordered_sets>::type,
        int>::value);

static_assert(
    std::is_same<tt::ContainerFinalElement<one_dim_set>::type, int>::value);
static_assert(
    std::is_same<tt::ContainerFinalElement<two_dim_set>::type, int>::value);
static_assert(
    std::is_same<tt::ContainerFinalElement<set_of_arrays>::type, int>::value);
static_assert(
    std::is_same<tt::ContainerFinalElement<set_of_vectors>::type, int>::value);

static_assert(!tt::kHasFixedDimensions<one_dim_vector>);
static_assert(!tt::kHasFixedDimensions<two_dim_vector>);
static_assert(!tt::kHasFixedDimensions<three_dim_vector>);

static_assert(tt::kHasFixedDimensions<one_dim_array>);
static_assert(tt::kHasFixedDimensions<two_dim_array>);
static_assert(tt::kHasFixedDimensions<three_dim_array>);

static_assert(!tt::kHasFixedDimensions<vector_of_arrays>);

static_assert(!tt::kHasFixedDimensions<unordered_set>);
static_assert(!tt::kHasFixedDimensions<vector_of_unordered_sets>);
static_assert(!tt::kHasFixedDimensions<array_of_vectors_of_unordered_sets>);

static_assert(!tt::kHasFixedDimensions<one_dim_set>);
static_assert(!tt::kHasFixedDimensions<two_dim_set>);
static_assert(!tt::kHasFixedDimensions<set_of_arrays>);
static_assert(!tt::kHasFixedDimensions<set_of_vectors>);

static_assert(std::is_same<std::integer_sequence<std::size_t, kDimOne>,
                           tt::FixedDimensions<one_dim_array>::type>::value);
static_assert(
    (std::is_same<std::integer_sequence<std::size_t, kDimTwo, kDimOne>,
                  tt::FixedDimensions<two_dim_array>::type>::value));
static_assert(std::is_same<
              std::integer_sequence<std::size_t, kDimThree, kDimTwo, kDimOne>,
              tt::FixedDimensions<three_dim_array>::type>::value);

static_assert(tt::kIsMappedToPg<one_dim_vector>);
static_assert(tt::kIsMappedToPg<two_dim_vector>);
static_assert(tt::kIsMappedToPg<three_dim_vector>);

static_assert(tt::kIsMappedToPg<one_dim_array>);
static_assert(tt::kIsMappedToPg<two_dim_array>);
static_assert(tt::kIsMappedToPg<three_dim_array>);

static_assert(tt::kIsMappedToPg<vector_of_arrays>);

static_assert(tt::kHasParser<one_dim_vector>);

static_assert(tt::kIsMappedToPg<unordered_set>);
static_assert(tt::kIsMappedToPg<vector_of_unordered_sets>);
static_assert(tt::kIsMappedToPg<array_of_vectors_of_unordered_sets>);

static_assert(tt::kIsMappedToPg<one_dim_set>);
static_assert(tt::kIsMappedToPg<two_dim_set>);
static_assert(tt::kIsMappedToPg<set_of_arrays>);
static_assert(tt::kIsMappedToPg<set_of_vectors>);

static_assert(tt::kTypeBufferCategory<one_dim_vector> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<two_dim_vector> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<three_dim_vector> ==
              io::BufferCategory::kArrayBuffer);

static_assert(tt::kTypeBufferCategory<one_dim_array> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<two_dim_array> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<three_dim_array> ==
              io::BufferCategory::kArrayBuffer);

static_assert(tt::kTypeBufferCategory<vector_of_arrays> ==
              io::BufferCategory::kArrayBuffer);

static_assert(tt::kTypeBufferCategory<unordered_set> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<vector_of_unordered_sets> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<array_of_vectors_of_unordered_sets> ==
              io::BufferCategory::kArrayBuffer);

static_assert(tt::kTypeBufferCategory<one_dim_set> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<two_dim_set> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<set_of_arrays> ==
              io::BufferCategory::kArrayBuffer);
static_assert(tt::kTypeBufferCategory<set_of_vectors> ==
              io::BufferCategory::kArrayBuffer);

static_assert(tt::kIsMappedToPg<io::detail::ContainerChunk<one_dim_vector>>);

}  // namespace static_test

namespace {

const pg::UserTypes types;

pg::io::TypeBufferCategory GetTestTypeCategories() {
  pg::io::TypeBufferCategory result;
  using Oids = pg::io::PredefinedOids;
  for (auto p_oid : {Oids::kInt2, Oids::kInt2Array, Oids::kInt4,
                     Oids::kInt4Array, Oids::kInt8, Oids::kInt8Array}) {
    auto oid = static_cast<pg::Oid>(p_oid);
    result.insert(std::make_pair(oid, io::GetBufferCategory(p_oid)));
  }
  return result;
}

const std::string kArraysSQL = R"~(
select  '{1, 2, 3, 4}'::integer[],
        '{{1}, {2}, {3}, {4}}'::integer[],
        '{{1, 2}, {3, 4}}'::integer[],
        '{{{1}, {2}}, {{3}, {4}}}'::integer[],
        '{1, 2}'::smallint[],
        '{1, 2}'::bigint[]
)~";

TEST(PostgreIO, Arrays) {
  const pg::io::TypeBufferCategory categories = GetTestTypeCategories();
  {
    static_test::one_dim_vector src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::one_dim_vector tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(src, tgt);
  }
  {
    static_test::one_dim_vector src{1, 2, 3};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::one_dim_vector tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(src, tgt);

    static_test::one_dim_array a1;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a1, categories));

    static_test::two_dim_array a2;
    UEXPECT_THROW(io::ReadBuffer(fb, a2, categories), pg::DimensionMismatch);

    static_test::three_dim_array a3;
    UEXPECT_THROW(io::ReadBuffer(fb, a3, categories), pg::DimensionMismatch);
  }
  {
    static_test::two_dim_vector src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::two_dim_vector tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(src, tgt);
  }
  {
    static_test::two_dim_vector src{{1, 2, 3}, {4, 5, 6}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::two_dim_vector tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(src, tgt);

    static_test::two_dim_array a2;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a2, categories));

    static_test::one_dim_array a1;
    UEXPECT_THROW(io::ReadBuffer(fb, a1, categories), pg::DimensionMismatch);
    static_test::three_dim_array a3;
    UEXPECT_THROW(io::ReadBuffer(fb, a3, categories), pg::DimensionMismatch);
  }
  {
    using test_array = static_test::three_dim_array;
    test_array src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_array tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(src, tgt);
  }
  {
    using test_array = static_test::three_dim_array;
    test_array src{{{{{{1, 2, 3}}, {{4, 5, 6}}}}}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_array tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(src, tgt);
  }
  {
    using test_array = static_test::vector_of_arrays;
    test_array src{{{{{1, 2, 3}}, {{4, 5, 6}}}}, {{{{1, 2, 3}}, {{4, 5, 6}}}}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_array tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(src, tgt);
  }
  {
    /*! [Invalid dimensions] */
    static_test::two_dim_vector src{{1, 2, 3}, {4, 5}};
    pg::test::Buffer buffer;
    UEXPECT_THROW(io::WriteBuffer(types, buffer, src), pg::InvalidDimensions);
    /*! [Invalid dimensions] */
  }
}

TEST(PostgreIO, ArraysSet) {
  const pg::io::TypeBufferCategory categories = GetTestTypeCategories();
  {
    static_test::one_dim_set src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::one_dim_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    static_test::one_dim_set src{1, 2, 3};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::one_dim_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);

    static_test::one_dim_array a1;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a1, categories));

    static_test::two_dim_array a2;
    UEXPECT_THROW(io::ReadBuffer(fb, a2, categories), pg::DimensionMismatch);

    static_test::three_dim_array a3;
    UEXPECT_THROW(io::ReadBuffer(fb, a3, categories), pg::DimensionMismatch);
  }
  {
    static_test::two_dim_set src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::two_dim_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    static_test::two_dim_set src{{1, 2, 3}, {4, 5, 6}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::two_dim_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);

    static_test::two_dim_array a2;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a2, categories));

    static_test::set_of_arrays a4;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a4, categories));

    static_test::set_of_vectors a5;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a5, categories));

    static_test::one_dim_set a1;
    UEXPECT_THROW(io::ReadBuffer(fb, a1, categories), pg::DimensionMismatch);
    static_test::three_dim_array a3;
    UEXPECT_THROW(io::ReadBuffer(fb, a3, categories), pg::DimensionMismatch);
  }
  {
    using test_set_of_vectors = static_test::set_of_vectors;
    test_set_of_vectors src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_set_of_vectors tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    using test_set_of_arrays = static_test::set_of_arrays;
    test_set_of_arrays src{{1, 2, 4}, {3, 4, 5}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_set_of_arrays tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    using test_set_of_vectors = static_test::set_of_vectors;
    test_set_of_vectors src{{1, 2, 4}, {3, 4, 5}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_set_of_vectors tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    static_test::one_dim_vector src{1, 2, 2, 3, 2, 3};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::one_dim_set expected(src.begin(), src.end());
    static_test::one_dim_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, expected);
  }
  {
    /*! [Invalid dimensions] */
    static_test::set_of_vectors src{{1, 2, 3}, {4, 5}};
    pg::test::Buffer buffer;
    UEXPECT_THROW(io::WriteBuffer(types, buffer, src), pg::InvalidDimensions);
    /*! [Invalid dimensions] */
  }
}

TEST(PostgreIO, ArraysUnorderedSet) {
  const pg::io::TypeBufferCategory categories = GetTestTypeCategories();
  {
    static_test::unordered_set src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::unordered_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    static_test::unordered_set src{1, 2, 3};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::unordered_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);

    static_test::one_dim_array a1;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a1, categories));

    static_test::two_dim_array a2;
    UEXPECT_THROW(io::ReadBuffer(fb, a2, categories), pg::DimensionMismatch);

    static_test::three_dim_array a3;
    UEXPECT_THROW(io::ReadBuffer(fb, a3, categories), pg::DimensionMismatch);
  }
  {
    static_test::vector_of_unordered_sets src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::vector_of_unordered_sets tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    static_test::vector_of_unordered_sets src{{1, 2, 3}, {4, 5, 6}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::vector_of_unordered_sets tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);

    static_test::two_dim_array a2;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, a2, categories));

    static_test::one_dim_array a1;
    UEXPECT_THROW(io::ReadBuffer(fb, a1, categories), pg::DimensionMismatch);
    static_test::three_dim_array a3;
    UEXPECT_THROW(io::ReadBuffer(fb, a3, categories), pg::DimensionMismatch);
  }
  {
    using test_array_of_vectors_of_unordered_sets =
        static_test::array_of_vectors_of_unordered_sets;
    test_array_of_vectors_of_unordered_sets src{};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_array_of_vectors_of_unordered_sets tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    using test_array = static_test::array_of_vectors_of_unordered_sets;
    test_array src{{{{1, 2}}, {{3, 4}}, {{5, 6}}}};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    // PrintBuffer(buffer);
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    test_array tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, src);
  }
  {
    static_test::one_dim_vector src{1, 2, 2, 3, 2, 3};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
    EXPECT_FALSE(buffer.empty());
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kArrayBuffer);
    static_test::unordered_set expected(src.begin(), src.end());
    static_test::unordered_set tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt, categories));
    EXPECT_EQ(tgt, expected);
  }
  {
    /*! [Invalid dimensions] */
    static_test::vector_of_unordered_sets src{{1, 2, 3}, {4, 5}};
    pg::test::Buffer buffer;
    UEXPECT_THROW(io::WriteBuffer(types, buffer, src), pg::InvalidDimensions);
    /*! [Invalid dimensions] */
  }
}

UTEST_P(PostgreConnection, ArrayRoundtrip) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  {
    using test_array = static_test::one_dim_vector;
    test_array src{-3, -2, 0, 1, 2, 3};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as int_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<int>(), pg::InvalidParserCategory);
    EXPECT_EQ(src, tgt);
  }
  {
    using test_array = static_test::vector_of_arrays;
    test_array src{{{{{1, 2, 3}}, {{4, 5, 6}}}}, {{{{1, 2, 3}}, {{4, 5, 6}}}}};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as array3d", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<int>(), pg::InvalidParserCategory);
    EXPECT_EQ(src, tgt);
  }
  {
    using test_array = std::vector<float>;
    test_array src{-3, -2, 0, 1, 2, 3};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as float_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<float>(), pg::InvalidParserCategory);
    EXPECT_EQ(src, tgt);
  }
  {
    using test_array = std::vector<std::string>;
    test_array src{"", "foo", "bar", ""};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as text_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
    EXPECT_EQ(src, tgt);
  }
  {
    using nullable_type = std::optional<std::string>;
    using test_array = std::vector<nullable_type>;
    test_array src{
        {}, nullable_type{"foo"}, nullable_type{"bar"}, nullable_type{""}};
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as text_array_with_nulls", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<nullable_type>(), pg::InvalidParserCategory);
    ASSERT_EQ(4, tgt.size());
    EXPECT_FALSE(tgt[0].has_value());
    EXPECT_TRUE(tgt[1].has_value());
    EXPECT_TRUE(tgt[2].has_value());
    EXPECT_TRUE(tgt[3].has_value());
    EXPECT_EQ(src, tgt);
    std::vector<std::string> tgt2;
    UEXPECT_THROW(res[0][0].To(tgt2), pg::TypeCannotBeNull);
  }
  {
    using test_array = std::vector<std::vector<std::string>>;
    test_array src{};
    UEXPECT_NO_THROW(res =
                         GetConn()->Execute("select $1 as text_2d_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
    EXPECT_EQ(src, tgt);
  }
}

UTEST_P(PostgreConnection, ArraySetRoundtrip) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  {
    using test_array = static_test::one_dim_set;
    test_array src{-3, -2, 0, 1, 2, 3};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as int_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<int>(), pg::InvalidParserCategory);
    EXPECT_EQ(tgt, src);
  }
  {
    using test_array = static_test::two_dim_set;
    test_array src{{1, 2, 3, 4, 5, 6}, {1, 2, 3, 4, 5, 7}};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as array3d", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<int>(), pg::InvalidParserCategory);
    EXPECT_EQ(tgt, src);
  }
  {
    using test_array = std::set<float>;
    test_array src{-3, -2, 0, 1, 2, 3};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as float_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<float>(), pg::InvalidParserCategory);
    EXPECT_EQ(tgt, src);
  }
  {
    using test_array = std::set<std::string>;
    test_array src{"", "foo", "bar", ""};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as text_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
    std::set<std::string> expected(src.begin(), src.end());
    EXPECT_EQ(tgt, expected);
  }
  {
    using nullable_type = std::optional<std::string>;
    using test_array = std::set<nullable_type>;
    test_array src{
        {}, nullable_type{"foo"}, nullable_type{"bar"}, nullable_type{""}};
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as text_array_with_nulls", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<nullable_type>(), pg::InvalidParserCategory);
    ASSERT_EQ(4, tgt.size());
    EXPECT_EQ(tgt, src);
    std::set<std::string> tgt2;
    UEXPECT_THROW(res[0][0].To(tgt2), pg::TypeCannotBeNull);
  }
  {
    using test_optional_array = std::optional<std::set<std::string>>;
    test_optional_array src = std::nullopt;
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as optional_text_array", src));
    test_optional_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    EXPECT_EQ(tgt, std::nullopt);
    std::set<std::string> tgt2;
    UEXPECT_THROW(res[0][0].To(tgt2), pg::FieldValueIsNull);
  }
  {
    using test_optional_array = std::optional<std::set<std::string>>;
    test_optional_array src = std::make_optional<std::set<std::string>>(
        {std::string{"foo"}, std::string{"bar"}, std::string{""}});
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as optional_text_array", src));
    test_optional_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    EXPECT_EQ(tgt, src);
    std::set<std::string> tgt2;
    UEXPECT_NO_THROW(res[0][0].To(tgt2));
    EXPECT_EQ(tgt2, *src);
  }
}

UTEST_P(PostgreConnection, ArrayUnorderedSetRoundtrip) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  {
    using test_array = static_test::unordered_set;
    test_array src{-3, -2, 0, 1, 2, 3};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as int_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<int>(), pg::InvalidParserCategory);
    EXPECT_EQ(tgt, src);
  }
  {
    using test_array = static_test::vector_of_unordered_sets;
    test_array src{{1, 2, 3, 4, 5, 6}, {1, 2, 3, 4, 5, 6}};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as array3d", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<int>(), pg::InvalidParserCategory);
    EXPECT_EQ(tgt, src);
  }
  {
    using test_array = std::unordered_set<float>;
    test_array src{-3, -2, 0, 1, 2, 3};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as float_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<float>(), pg::InvalidParserCategory);
    EXPECT_EQ(tgt, src);
  }
  {
    using test_array = std::unordered_set<std::string>;
    test_array src{"", "foo", "bar", ""};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as text_array", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<std::string>(), pg::InvalidParserCategory);
    std::unordered_set<std::string> expected(src.begin(), src.end());
    EXPECT_EQ(tgt, expected);
  }
  {
    using nullable_type = std::optional<std::string>;
    using test_array = std::unordered_set<nullable_type>;
    test_array src{
        {}, nullable_type{"foo"}, nullable_type{"bar"}, nullable_type{""}};
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as text_array_with_nulls", src));
    test_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    UEXPECT_THROW(res[0][0].As<nullable_type>(), pg::InvalidParserCategory);
    ASSERT_EQ(4, tgt.size());
    EXPECT_EQ(tgt, src);
    std::unordered_set<std::string> tgt2;
    UEXPECT_THROW(res[0][0].To(tgt2), pg::TypeCannotBeNull);
  }
  {
    using test_optional_array = std::optional<std::unordered_set<std::string>>;
    test_optional_array src = std::nullopt;
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as optional_text_array", src));
    test_optional_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    EXPECT_EQ(tgt, std::nullopt);
    std::unordered_set<std::string> tgt2;
    UEXPECT_THROW(res[0][0].To(tgt2), pg::FieldValueIsNull);
  }
  {
    using test_optional_array = std::optional<std::unordered_set<std::string>>;
    test_optional_array src =
        std::make_optional<std::unordered_set<std::string>>(
            {std::string{"foo"}, std::string{"bar"}, std::string{""}});
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as optional_text_array", src));
    test_optional_array tgt;
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    EXPECT_EQ(tgt, src);
    std::unordered_set<std::string> tgt2;
    UEXPECT_NO_THROW(res[0][0].To(tgt2));
    EXPECT_EQ(tgt2, *src);
  }
}

UTEST_P(PostgreConnection, ArrayEmpty) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  {
    UEXPECT_NO_THROW(res = GetConn()->Execute("select ARRAY[]::integer[]"));
    static_test::one_dim_vector v{1};
    UEXPECT_NO_THROW(res[0].To(v));
    EXPECT_TRUE(v.empty());
  }
  {
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as array",
                                              static_test::one_dim_vector{}));
    static_test::one_dim_vector v{1};
    UEXPECT_NO_THROW(res[0].To(v));
    EXPECT_TRUE(v.empty());
  }
}

UTEST_P(PostgreConnection, ArrayOfVarchar) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(GetConn()->Execute(
      "create temporary table vchar_array_test( v varchar[] )"));
  UEXPECT_NO_THROW(
      GetConn()->Execute("insert into vchar_array_test values ($1)",
                         std::vector<std::string>{"foo", "bar"}));
}

UTEST_P(PostgreConnection, ArrayOfBool) {
  CheckConnection(GetConn());
  std::vector<bool> src{true, false, true};
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1::boolean[]",
                               std::vector<bool>{true, false, true}));
  std::vector<bool> tgt;
  UEXPECT_NO_THROW(res[0][0].To(tgt));
  EXPECT_EQ(src, tgt);
}

void CheckSplit(const io::detail::ContainerSplitter<std::vector<int>>& split) {
  const auto& data = split.GetContainer();

  EXPECT_EQ(data.empty(), split.empty());
  auto expected_chunks = data.size() / split.ChunkSize() +
                         (data.size() % split.ChunkSize() ? 1 : 0);
  EXPECT_EQ(expected_chunks, split.size());

  std::size_t elem_cnt{0};
  std::size_t chunk_cnt{0};

  for (auto chunk : split) {
    ++chunk_cnt;
    elem_cnt += chunk.size();
  }

  EXPECT_EQ(data.size(), elem_cnt);
  EXPECT_EQ(expected_chunks, chunk_cnt);
}

TEST(PostgreIO, SplitContainer) {
  std::vector<int> data(1000, 42);
  CheckSplit(io::SplitContainer(data, 10));
  data.push_back(42);
  CheckSplit(io::SplitContainer(data, 10));
  data.resize(9);
  CheckSplit(io::SplitContainer(data, 10));
  data.clear();
  CheckSplit(io::SplitContainer(data, 10));
}

UTEST_P(PostgreConnection, ChunkedContainer) {
  CheckConnection(GetConn());

  GetConn()->Execute("create temporary table chunked_array_test(v integer)");
  std::vector<int> data(1001, 42);
  auto split = io::SplitContainer(data, 100);
  for (auto chunk : split) {
    UEXPECT_NO_THROW(GetConn()->Execute(
        "insert into chunked_array_test select * from unnest($1)", chunk));
  }

  auto res = GetConn()->Execute("select count(*) from chunked_array_test");
  EXPECT_EQ(data.size(), res.Front().As<pg::Bigint>(pg::kFieldTag));
}

UTEST_P(PostgreConnection, TransactionChunkedContainer) {
  CheckConnection(GetConn());

  GetConn()->Execute("create temporary table chunked_array_test(v integer)");
  std::vector<int> data(1001, 42);

  pg::Transaction trx(std::move(GetConn()), pg::TransactionOptions{});
  UEXPECT_NO_THROW(trx.ExecuteBulk(
      "insert into chunked_array_test select * from unnest($1)", data, 100));

  auto res = trx.Execute("select count(*) from chunked_array_test");
  EXPECT_EQ(data.size(), res.Front().As<pg::Bigint>(pg::kFieldTag));

  trx.Commit();
}

namespace {
/// [ExecuteDecomposeBulk]
struct IdAndValue final {
  int id{};
  std::string value;
};

void InsertDecomposedInChunks(pg::Transaction& trx,
                              const std::vector<IdAndValue>& rows) {
  trx.ExecuteDecomposeBulk(
      "INSERT INTO decomposed_chunked_array_test(id, value) "
      "VALUES(UNNEST($1), UNNEST($2))",
      rows);
}
/// [ExecuteDecomposeBulk]

UTEST_P(PostgreConnection, TransactionDecomposedChunkedContainer) {
  CheckConnection(GetConn());

  GetConn()->Execute(
      "create temporary table decomposed_chunked_array_test("
      "id integer, value text)");

  std::vector<IdAndValue> rows_to_insert(1001);
  int index = 0;
  std::generate(rows_to_insert.begin(), rows_to_insert.end(), [&index] {
    const auto x = index++;
    return IdAndValue{x, std::to_string(x)};
  });

  pg::Transaction trx{std::move(GetConn())};
  UEXPECT_NO_THROW(InsertDecomposedInChunks(trx, rows_to_insert));
  const auto inserted_rows =
      trx.Execute("SELECT id, value FROM decomposed_chunked_array_test")
          .AsContainer<std::vector<IdAndValue>>(pg::kRowTag);

  // we don't define operator== on IdAndValue to not bloat the sample (and that
  // operator could confuse people, like why is it needed there), so this
  ASSERT_EQ(rows_to_insert.size(), inserted_rows.size());
  for (std::size_t i = 0; i < rows_to_insert.size(); ++i) {
    const auto eq = [](const auto& lhs, const auto& rhs) {
      return lhs.id == rhs.id && lhs.value == rhs.value;
    };
    ASSERT_TRUE(eq(rows_to_insert[i], inserted_rows[i]));
  }

  trx.Commit();
}
}  // namespace

}  // namespace

USERVER_NAMESPACE_END

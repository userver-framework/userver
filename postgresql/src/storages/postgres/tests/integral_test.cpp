#include <gtest/gtest.h>

#include <limits>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

namespace tt = io::traits;

const pg::UserTypes types;

template <typename Int>
class PostgreIOIntegral : public ::testing::Test {};

using IntTypes = ::testing::Types<pg::Smallint, pg::Integer, pg::Bigint, short, int, long, long long>;

template <typename TPgType, typename TValueType>
struct NarrowingTypes {
    using PgType = TPgType;
    using ValueType = TValueType;
};

template <typename NarrowingTypes>
class PostgreIOIntegralNarrowing : public ::testing::Test {
    using PgType = typename NarrowingTypes::PgType;
    using ValueType = typename NarrowingTypes::ValueType;
};

using NarrowingTestTypes = ::testing::Types<
    NarrowingTypes<pg::Integer, pg::Smallint>,
    NarrowingTypes<pg::Bigint, pg::Smallint>,
    NarrowingTypes<pg::Bigint, pg::Integer> >;

}  // namespace

TEST(PostgreIOIntegral, ParserRegistry) {
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt2));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt4));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kInt8));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kOid));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kTid));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kXid));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kCid));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kLsn));
}

TEST(PostgreIOIntegral, Bool) {
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, true));
    auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
    bool tgt{false};
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_TRUE(tgt);
}

TYPED_TEST_SUITE(PostgreIOIntegral, IntTypes);

TYPED_TEST(PostgreIOIntegral, Int) {
    static_assert(tt::kIsMappedToPg<TypeParam>, "missing mapping");
    static_assert(tt::kHasFormatter<TypeParam>, "missing binary formatter");
    static_assert(tt::kHasParser<TypeParam>, "missing binary parser");
    using Limits = std::numeric_limits<TypeParam>;

    for (const TypeParam src :
         {Limits::lowest(), TypeParam(-42), TypeParam(-1), TypeParam(0), TypeParam(1), TypeParam(42), Limits::max()})
    {
        pg::test::Buffer buffer;
        UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
        auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
        TypeParam tgt{0};
        UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
        EXPECT_EQ(src, tgt);
    }
}

TYPED_TEST_SUITE(PostgreIOIntegralNarrowing, NarrowingTestTypes);

TYPED_TEST(PostgreIOIntegralNarrowing, Success) {
    using ValueType = typename TypeParam::ValueType;
    using Limits = std::numeric_limits<ValueType>;

    for (const ValueType src :
         {Limits::lowest(), ValueType(-42), ValueType(-1), ValueType(0), ValueType(1), ValueType(42), Limits::max()})
    {
        pg::test::Buffer buffer;
        const typename TypeParam::PgType output = src;
        UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, output));
        auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
        ValueType tgt{0};
        UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
        EXPECT_EQ(src, tgt);
    }
}

TYPED_TEST(PostgreIOIntegralNarrowing, Fail) {
    using ValueType = typename TypeParam::ValueType;
    using PgType = typename TypeParam::PgType;
    using Limits = std::numeric_limits<ValueType>;

    for (const PgType src : {static_cast<PgType>(Limits::lowest()) - 1, static_cast<PgType>(Limits::max()) + 1}) {
        pg::test::Buffer buffer;
        UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
        auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
        ValueType tgt{0};
        UEXPECT_THROW(io::ReadBuffer(fb, tgt), storages::postgres::NarrowingOverflow);
    }
}

USERVER_NAMESPACE_END

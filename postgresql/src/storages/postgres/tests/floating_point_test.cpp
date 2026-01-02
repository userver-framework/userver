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

template <typename Float>
class PostgreIOFloating : public ::testing::Test {};

using FloatTypes = ::testing::Types<float, double>;

}  // namespace

TEST(PostgreIOFloating, ParserRegistry) {
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kFloat4));
    EXPECT_TRUE(io::HasParser(io::PredefinedOids::kFloat8));
}

TYPED_TEST_SUITE(PostgreIOFloating, FloatTypes);

TYPED_TEST(PostgreIOFloating, Float) {
    static_assert(tt::kIsMappedToPg<TypeParam>, "missing mapping");
    static_assert(tt::kHasFormatter<TypeParam>, "missing binary formatter");
    static_assert(tt::kHasParser<TypeParam>, "missing binary parser");
    using Limits = std::numeric_limits<TypeParam>;

    for (const TypeParam src :
         {Limits::lowest(),
          TypeParam(-42.0),
          TypeParam(-1.1),
          TypeParam(0.),
          TypeParam(1.5),
          TypeParam(36.6),
          Limits::max()})
    {
        pg::test::Buffer buffer;
        UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
        auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
        TypeParam tgt{0};
        UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
        EXPECT_EQ(src, tgt);
    }
}

TEST(PostgreIOFloating, NarrowingSuccess) {
    using Limits = std::numeric_limits<float>;

    for (const float src : {Limits::lowest(), -42.0f, -1.1f, 0.f, 1.5f, 36.6f, Limits::max()}) {
        pg::test::Buffer buffer;
        const double output = src;
        UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, output));
        auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
        float tgt{0};
        UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
        EXPECT_EQ(src, tgt);
    }
}

TEST(PostgreIOFloating, NarrowingFail) {
    using Limits = std::numeric_limits<float>;

    for (const double src : {2.0 * Limits::lowest(), 2.0 * Limits::max()}) {
        pg::test::Buffer buffer;
        UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src));
        auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
        float tgt{0};
        UEXPECT_THROW(io::ReadBuffer(fb, tgt), storages::postgres::NarrowingOverflow);
    }
}

USERVER_NAMESPACE_END

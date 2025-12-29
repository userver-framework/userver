#include <cmath>
#include <limits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <userver/utest/assert_macros.hpp>

#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename TFrom, typename TTo>
struct CastTypes {
    using From = TFrom;
    using To = TTo;
};

using FloatingPointCastTypes = ::testing::Types<
    CastTypes<float, float>,
    CastTypes<float, double>,
    CastTypes<float, long double>,
    CastTypes<double, float>,
    CastTypes<double, double>,
    CastTypes<double, long double>,
    CastTypes<long double, float>,
    CastTypes<long double, double>,
    CastTypes<long double, long double>>;

template <typename CastTypes>
class NumericCastFloatingPoint : public ::testing::Test {
    using From = typename CastTypes::From;
    using To = typename CastTypes::To;
};

TEST(NumericCast, CompileTime) {
    static_assert(utils::numeric_cast<unsigned int>(1) == 1u);
    static_assert(utils::numeric_cast<float>(1.0) == 1.0f);
    static_assert(utils::numeric_cast<double>(1.0f) == 1.0);
    static_assert(utils::numeric_cast<long double>(1.0) == 1.0L);
}

TEST(NumericCast, Smoke) {
    EXPECT_EQ(utils::numeric_cast<unsigned int>(1), 1u);
    EXPECT_EQ(utils::numeric_cast<std::size_t>(1), static_cast<std::size_t>(1));

    EXPECT_EQ(utils::numeric_cast<float>(1.0f), 1.0f);
    EXPECT_EQ(utils::numeric_cast<float>(1.0), 1.0f);
    EXPECT_EQ(utils::numeric_cast<float>(1.0L), 1.0f);

    EXPECT_EQ(utils::numeric_cast<double>(1.0f), 1.0);
    EXPECT_EQ(utils::numeric_cast<double>(1.0), 1.0);
    EXPECT_EQ(utils::numeric_cast<double>(1.0L), 1.0);

    EXPECT_EQ(utils::numeric_cast<long double>(1.0f), 1.0L);
    EXPECT_EQ(utils::numeric_cast<long double>(1.0), 1.0L);
    EXPECT_EQ(utils::numeric_cast<long double>(1.0L), 1.0L);
}

TYPED_TEST_SUITE(NumericCastFloatingPoint, FloatingPointCastTypes);

TYPED_TEST(NumericCastFloatingPoint, Nan) {
    using Limits = std::numeric_limits<typename TypeParam::From>;

    if constexpr (Limits::has_quiet_NaN) {
        EXPECT_TRUE(std::isnan(utils::numeric_cast<typename TypeParam::To>(Limits::quiet_NaN())));
    }
}

TYPED_TEST(NumericCastFloatingPoint, Infinity) {
    using To = typename TypeParam::To;
    using FromLimits = std::numeric_limits<typename TypeParam::From>;
    using ToLimits = std::numeric_limits<To>;

    if constexpr (FromLimits::has_infinity) {
        // +inf
        EXPECT_EQ(utils::numeric_cast<To>(FromLimits::infinity()), ToLimits::infinity());
        // -inf
        EXPECT_EQ(utils::numeric_cast<To>(-FromLimits::infinity()), -ToLimits::infinity());
    }
}

TEST(NumericCast, SignedToUnsignedOverflow) {
    /// [Sample utils::numeric_cast usage]
    EXPECT_EQ(utils::numeric_cast<std::uint16_t>(0xffff), 0xffffu);
    EXPECT_THROW(utils::numeric_cast<std::uint16_t>(0x10000), std::runtime_error);

    EXPECT_EQ(utils::numeric_cast<std::uint16_t>(0), 0);
    EXPECT_THROW(utils::numeric_cast<std::uint16_t>(-1), std::runtime_error);
    /// [Sample utils::numeric_cast usage]
}

TEST(NumericCast, UnsignedToSignedOverflow) {
    EXPECT_EQ(utils::numeric_cast<std::int16_t>(0), 0);
    EXPECT_THROW(utils::numeric_cast<std::int16_t>(0xffff), std::runtime_error);

    EXPECT_EQ(utils::numeric_cast<std::int16_t>(0x7fff), 0x7fff);
    EXPECT_THROW(utils::numeric_cast<std::int16_t>(0x8000), std::runtime_error);
}

TEST(NumericCast, FloatingPointOverflow) {
    using FloatLimits = std::numeric_limits<float>;

    EXPECT_EQ(utils::numeric_cast<float>(double(FloatLimits::max())), FloatLimits::max());
    EXPECT_THROW(utils::numeric_cast<float>(2.0 * FloatLimits::max()), std::runtime_error);
    EXPECT_EQ(utils::numeric_cast<float>(static_cast<long double>(FloatLimits::max())), FloatLimits::max());
    EXPECT_THROW(utils::numeric_cast<float>(2.0L * FloatLimits::max()), std::runtime_error);

    EXPECT_EQ(utils::numeric_cast<float>(double(FloatLimits::lowest())), FloatLimits::lowest());
    EXPECT_THROW(utils::numeric_cast<float>(2.0 * FloatLimits::lowest()), std::runtime_error);
    EXPECT_EQ(utils::numeric_cast<float>(static_cast<long double>(FloatLimits::lowest())), FloatLimits::lowest());
    EXPECT_THROW(utils::numeric_cast<float>(2.0L * FloatLimits::lowest()), std::runtime_error);

    using DoubleLimits = std::numeric_limits<double>;

    EXPECT_EQ(utils::numeric_cast<double>(static_cast<long double>(DoubleLimits::max())), DoubleLimits::max());
    EXPECT_EQ(utils::numeric_cast<double>(static_cast<long double>(DoubleLimits::lowest())), DoubleLimits::lowest());
    if constexpr (sizeof(double) < sizeof(long double)) {
        EXPECT_THROW(utils::numeric_cast<double>(2.0L * DoubleLimits::max()), std::runtime_error);
        EXPECT_THROW(utils::numeric_cast<double>(2.0L * DoubleLimits::lowest()), std::runtime_error);
    }
}

}  // namespace

USERVER_NAMESPACE_END

#include <userver/utils/flags.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <initializer_list>
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace {

enum class Enum {
    kNone = 0,
    kFirst = 1 << 0,
    kSecond = 1 << 1,
};
using enum Enum;

using ValueType = std::underlying_type_t<Enum>;

using Flags = utils::Flags<Enum>;
using AtomicFlags = utils::AtomicFlags<Enum>;

}  // namespace

TEST(FlagsTest, FlagsConstructorDefault) {
    Flags f;
    EXPECT_EQ(f.GetValue(), 0);
}

TEST(FlagsTest, FlagsConstructorEnum) {
    Flags f(kFirst);
    EXPECT_EQ(f.GetValue(), 1);
}

TEST(FlagsTest, FlagsConstructorList) {
    constexpr int expected = 0b11;

    Flags f1{kFirst, kSecond};
    EXPECT_EQ(f1.GetValue(), expected);

    Flags f2{kSecond, kFirst};
    EXPECT_EQ(f2.GetValue(), expected);
}

TEST(FlagsTest, FlagsConstructorCopyMove) {
    Flags f1{kFirst};

    auto f2 = f1;
    EXPECT_EQ(f2.GetValue(), 1);

    auto f3 = std::move(f1);
    EXPECT_EQ(f3.GetValue(), 1);
}

TEST(FlagsTest, FlagsAssignment) {
    Flags f1;
    Flags f2(kFirst);

    f1 = f2;
    EXPECT_EQ(f1.GetValue(), 1);

    f1 = Flags();
    EXPECT_EQ(f1.GetValue(), 0);

    f1 = kFirst;
    EXPECT_EQ(f1.GetValue(), 1);
}

TEST(FlagsTest, FlagsSetValue) {
    Flags f;

    f.SetValue(42);
    EXPECT_EQ(f.GetValue(), 42);

    f.SetValue(0);
    EXPECT_EQ(f.GetValue(), 0);
}

TEST(FlagsTest, FlagsClear) {
    const auto flags = {kFirst, kSecond};

    Flags f{flags};
    EXPECT_EQ(f.Clear(kFirst).GetValue(), 2);
    EXPECT_EQ(f.Clear(kSecond).GetValue(), 0);
    
    f = Flags{flags};
    EXPECT_EQ(f.Clear(flags).GetValue(), 0);
}

TEST(FlagsTest, FlagsOpertorBool) {
    Flags f;
    EXPECT_FALSE(f);

    f.SetValue(1);
    EXPECT_TRUE(f);

    f.SetValue(42);
    EXPECT_TRUE(f);

    f.SetValue(0);
    EXPECT_FALSE(f);
}

TEST(FlagsTest, FlagsComparasion) {
    Flags f1(kFirst);
    Flags f2(kSecond);

    EXPECT_TRUE(f1 == f1);
    EXPECT_TRUE(f1 == kFirst);
    EXPECT_TRUE(kFirst == f1);
    EXPECT_FALSE(f1 == f2);
    EXPECT_FALSE(f1 == kSecond);
    EXPECT_FALSE(kSecond == f1);

    EXPECT_FALSE(f1 != f1);
    EXPECT_FALSE(f1 != kFirst);
    EXPECT_FALSE(kFirst != f1);
    EXPECT_TRUE(f1 != f2);
    EXPECT_TRUE(f1 != kSecond);
    EXPECT_TRUE(kSecond != f1);
}

TEST(FlagsTest, FlagsBitOr) {
    const auto flags = {kFirst, kSecond};

    Flags f;
    EXPECT_EQ(f | kFirst, kFirst);
    EXPECT_EQ(kFirst | f, kFirst);
    EXPECT_EQ(f |= kFirst, kFirst);
    f.SetValue(0);
    EXPECT_EQ((f |= kFirst) |= kSecond, flags);
}

TEST(FlagsTest, FlagsBitAnd) {
    const auto flags = {kFirst, kSecond};
    Flags f{flags};
    EXPECT_EQ(f & kFirst, kFirst);
    EXPECT_EQ(kFirst & f, kFirst);
    EXPECT_EQ(f &= kFirst, kFirst);
    f = Flags{flags};
    EXPECT_EQ((f &= kFirst) &= kSecond, kNone);
}

TEST(FlagsTest, AtomicConstructorDefault) {
    AtomicFlags f;
    EXPECT_EQ(f.GetValue(), 0);
}

TEST(FlagsTest, AtomicConstructorFlags) {
    AtomicFlags f(Flags{kFirst});
    EXPECT_EQ(f.GetValue(), 1);
}

TEST(FlagsTest, AtomicConstructorEnum) {
    AtomicFlags f(kFirst);
    EXPECT_EQ(f.GetValue(), 1);
}

TEST(FlagsTest, AtomicConstructorList) {
    constexpr int expected = 0b11;

    AtomicFlags f1{kFirst, kSecond};
    EXPECT_EQ(f1.GetValue(), expected);

    AtomicFlags f2{kSecond, kFirst};
    EXPECT_EQ(f2.GetValue(), expected);
}

TEST(FlagsTest, AtomicAssignment) {
    AtomicFlags f;

    f = kFirst;
    EXPECT_EQ(f.GetValue(), 1);

    f = kSecond;
    EXPECT_EQ(f.GetValue(), 2);
}

TEST(FlagsTest, AtomicOpertorBool) {
    AtomicFlags f;
    EXPECT_FALSE(f);

    f = kFirst;
    EXPECT_TRUE(f);

    f = kSecond;
    EXPECT_TRUE(f);

    f = kNone;
    EXPECT_FALSE(f);
}

TEST(FlagsTest, AtomicClear) {
    const auto flags = {kFirst, kSecond};

    AtomicFlags f{flags};
    EXPECT_EQ(f.Clear(kFirst).GetValue(), 2);
    EXPECT_EQ(f.Clear(kSecond).GetValue(), 0);
    
    f = Flags{flags};
    EXPECT_EQ(f.Clear(flags).GetValue(), 0);
}

TEST(FlagsTest, AtomicLoadStore) {
    AtomicFlags af{kFirst};
    EXPECT_EQ(af.Load(), kFirst);
    EXPECT_EQ(af.Store(kSecond).Load(), kSecond);
}

TEST(FlagsTest, AtomicComparasion) {
    AtomicFlags f1(kFirst);
    AtomicFlags f2(kSecond);

    EXPECT_TRUE(f1 == f1);
    EXPECT_TRUE(f1 == kFirst);
    EXPECT_TRUE(kFirst == f1);
    EXPECT_FALSE(f1 == f2);
    EXPECT_FALSE(f1 == kSecond);
    EXPECT_FALSE(kSecond == f1);

    EXPECT_FALSE(f1 != f1);
    EXPECT_FALSE(f1 != kFirst);
    EXPECT_FALSE(kFirst != f1);
    EXPECT_TRUE(f1 != f2);
    EXPECT_TRUE(f1 != kSecond);
    EXPECT_TRUE(kSecond != f1);
}

TEST(FlagsTest, AtomicBitOr) {
    const auto flags = {kFirst, kSecond};

    AtomicFlags f;
    EXPECT_EQ(f | kFirst, kFirst);
    EXPECT_EQ(kFirst | f, kFirst);
    EXPECT_EQ(f |= kFirst, kFirst);
    f = kNone;
    EXPECT_EQ((f |= kFirst) |= kSecond, flags);
}

TEST(FlagsTest, AtomicBitAnd) {
    const auto flags = {kFirst, kSecond};
    AtomicFlags f{flags};
    EXPECT_EQ(f & kFirst, kFirst);
    EXPECT_EQ(kFirst & f, kFirst);
    EXPECT_EQ(f &= kFirst, kFirst);
    f = Flags{flags};
    EXPECT_EQ((f &= kFirst) &= kSecond, kNone);
}

TEST(FlagsTest, AtomicFetch) {
    AtomicFlags f;

    EXPECT_EQ(f.FetchOr(kFirst), kNone);
    EXPECT_EQ(f.FetchOr(kSecond), kFirst);
    EXPECT_EQ(f.FetchAnd(kFirst).GetValue(), 0b11);
    EXPECT_EQ(f.FetchAnd(kSecond), kFirst);
    EXPECT_FALSE(f);
}

TEST(FlagsTest, AtomicExchange) {
    AtomicFlags f;
    EXPECT_EQ(f.Exchange(kFirst), kNone);
    EXPECT_EQ(f.Exchange(kSecond), kFirst);
    EXPECT_EQ(f.Exchange(kNone), kSecond);
    EXPECT_FALSE(f);
}

TEST(FlagsTest, AtomicCompareExchangeWeak) {
    AtomicFlags f;
    Flags expected;

    EXPECT_TRUE(f.CompareExchangeWeak(expected, kFirst));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kNone);

    EXPECT_FALSE(f.CompareExchangeWeak(expected, kNone));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kFirst);
}

TEST(FlagsTest, AtomicCompareExchangeWeakOrder) {
    constexpr auto order = std::memory_order_seq_cst;

    AtomicFlags f;
    Flags expected;

    EXPECT_TRUE(f.CompareExchangeWeak(expected, kFirst, order, order));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kNone);

    EXPECT_FALSE(f.CompareExchangeWeak(expected, kNone, order, order));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kFirst);
}

TEST(FlagsTest, AtomicCompareExchangeStrong) {
    AtomicFlags f;
    Flags expected;

    EXPECT_TRUE(f.CompareExchangeStrong(expected, kFirst));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kNone);

    EXPECT_FALSE(f.CompareExchangeStrong(expected, kNone));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kFirst);
}

TEST(FlagsTest, AtomicCompareExchangeStrongOrder) {
    constexpr auto order = std::memory_order_seq_cst;

    AtomicFlags f;
    Flags expected;

    EXPECT_TRUE(f.CompareExchangeStrong(expected, kFirst, order, order));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kNone);

    EXPECT_FALSE(f.CompareExchangeStrong(expected, kNone, order, order));
    EXPECT_EQ(f.Load(), kFirst);
    EXPECT_EQ(expected, kFirst);
}

USERVER_NAMESPACE_END

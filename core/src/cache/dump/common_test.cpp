#include <cache/dump/common.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include <formats/json/inline.hpp>
#include <formats/json/serialize.hpp>
#include <utest/utest.hpp>

#include <cache/dump/test_helpers.hpp>

namespace {

template <typename T>
void TestWriteReadEquals(const T& value) {
  EXPECT_EQ(cache::dump::WriteRead(value), value);
}

}  // namespace

template <typename T>
class CacheDumpCommonNumeric : public testing::Test {
 public:
  using Num = T;
};

using NumericTestTypes =
    ::testing::Types<int, std::uint8_t, std::int8_t, std::uint16_t,
                     std::int16_t, std::uint32_t, std::int32_t, std::uint64_t,
                     std::int64_t, float, double, long double>;
TYPED_TEST_SUITE(CacheDumpCommonNumeric, NumericTestTypes);

TYPED_TEST(CacheDumpCommonNumeric, Numeric) {
  using Num = typename TestFixture::Num;

  TestWriteReadEquals(Num{0});
  TestWriteReadEquals(Num{42});
  TestWriteReadEquals(std::numeric_limits<Num>::min());
  TestWriteReadEquals(std::numeric_limits<Num>::max());

  if constexpr (std::is_floating_point_v<Num>) {
    TestWriteReadEquals(-std::numeric_limits<Num>::min());
    TestWriteReadEquals(-std::numeric_limits<Num>::max());
    TestWriteReadEquals(std::numeric_limits<Num>::denorm_min());
    TestWriteReadEquals(-std::numeric_limits<Num>::denorm_min());
    TestWriteReadEquals(std::numeric_limits<Num>::infinity());
    TestWriteReadEquals(-std::numeric_limits<Num>::infinity());
    EXPECT_TRUE(std::isnan(
        cache::dump::WriteRead(std::numeric_limits<Num>::quiet_NaN())));
  } else {
    std::default_random_engine engine(42);
    std::uniform_int_distribution<Num> distribution(
        std::numeric_limits<Num>::min(), std::numeric_limits<Num>::max());
    for (int i = 0; i < 20; ++i) {
      TestWriteReadEquals(distribution(engine));
    }
  }
}

TEST(CacheDumpCommon, IntegerSizes) {
  EXPECT_EQ(cache::dump::ToBinary(0).size(), 1);
  EXPECT_EQ(cache::dump::ToBinary(42).size(), 1);
  EXPECT_EQ(cache::dump::ToBinary(127).size(), 1);
  EXPECT_EQ(cache::dump::ToBinary(128).size(), 2);
  EXPECT_EQ(cache::dump::ToBinary(0x3fff).size(), 2);
  EXPECT_EQ(cache::dump::ToBinary(0x4fff).size(), 4);
  EXPECT_EQ(cache::dump::ToBinary(0x3eff'ffff).size(), 4);
  EXPECT_EQ(cache::dump::ToBinary(0x4f00'0000).size(), 9);
  EXPECT_EQ(cache::dump::ToBinary(0xffff'ffff'ffff'ffff).size(), 9);

  EXPECT_EQ(cache::dump::ToBinary(-1).size(), 9);
}

TEST(CacheDumpCommon, String) {
  TestWriteReadEquals(std::string{""});
  TestWriteReadEquals(std::string{"a"});
  TestWriteReadEquals(std::string{"abc"});
  TestWriteReadEquals(std::string{"A big brown hog jumps over the lazy dog"});
}

TEST(CacheDumpCommon, Bool) {
  TestWriteReadEquals(false);
  TestWriteReadEquals(true);
}

struct TwoStrings {
  TwoStrings(std::string_view a, std::string_view b) : a(a), b(b) {}

  std::string a;
  std::string b;

  bool operator==(const TwoStrings& other) const {
    return a == other.a && b == other.b;
  }
};

void Write(cache::dump::Writer& writer, const TwoStrings& value) {
  writer.Write(value.a.size());
  writer.Write(value.b.size());
  writer.WriteRaw(value.a);
  writer.WriteRaw(value.b);
}

TwoStrings Read(cache::dump::Reader& reader, cache::dump::To<TwoStrings>) {
  // We can't just read one `string_view` after another. We could have copied
  // the first `string_view` to a `string`, but this example is being inventive.
  const auto size_a = reader.Read<std::size_t>();
  const auto size_b = reader.Read<std::size_t>();
  const auto ab = reader.ReadRaw(size_a + size_b);
  return TwoStrings{ab.substr(0, size_a), ab.substr(size_a, size_b)};
}

TEST(CacheDumpCommon, StringViewRaw) {
  TestWriteReadEquals(TwoStrings{"", "abc"});
}

namespace formats::json {

void Write(cache::dump::Writer& writer, const formats::json::Value& value) {
  // formats::json::ToString does not currently produce std::string_view, but
  // could be modified to do so.
  writer.Write(formats::json::ToString(value));
}

formats::json::Value Read(cache::dump::Reader& reader,
                          cache::dump::To<formats::json::Value>) {
  return formats::json::FromString(reader.Read<std::string_view>());
}

}  // namespace formats::json

TEST(CacheDumpCommon, StringView) {
  TestWriteReadEquals(formats::json::MakeObject("foo", 42, "bar", "baz"));
}

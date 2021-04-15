#include <dump/common.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include <boost/uuid/random_generator.hpp>

#include <formats/json/inline.hpp>
#include <formats/json/serialize.hpp>
#include <fs/blocking/write.hpp>
#include <utest/utest.hpp>

#include <dump/operations_mock.hpp>
#include <dump/test_helpers.hpp>

using dump::TestWriteReadCycle;

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

  TestWriteReadCycle(Num{0});
  TestWriteReadCycle(Num{42});
  TestWriteReadCycle(std::numeric_limits<Num>::min());
  TestWriteReadCycle(std::numeric_limits<Num>::max());

  if constexpr (std::is_floating_point_v<Num>) {
    TestWriteReadCycle(-std::numeric_limits<Num>::min());
    TestWriteReadCycle(-std::numeric_limits<Num>::max());
    TestWriteReadCycle(std::numeric_limits<Num>::denorm_min());
    TestWriteReadCycle(-std::numeric_limits<Num>::denorm_min());
    TestWriteReadCycle(std::numeric_limits<Num>::infinity());
    TestWriteReadCycle(-std::numeric_limits<Num>::infinity());
    EXPECT_TRUE(std::isnan(dump::FromBinary<Num>(
        dump::ToBinary<Num>(std::numeric_limits<Num>::quiet_NaN()))));
  } else {
    std::default_random_engine engine(42);
    std::uniform_int_distribution<Num> distribution(
        std::numeric_limits<Num>::min(), std::numeric_limits<Num>::max());
    for (int i = 0; i < 1000; ++i) {
      TestWriteReadCycle(distribution(engine));
    }
  }
}

TEST(CacheDumpCommon, IntegerSizes) {
  using dump::ToBinary;

  EXPECT_EQ(ToBinary(0).size(), 1);
  EXPECT_EQ(ToBinary(42).size(), 1);
  EXPECT_EQ(ToBinary(127).size(), 1);
  EXPECT_EQ(ToBinary(128).size(), 2);
  EXPECT_EQ(ToBinary(0x3fff).size(), 2);
  EXPECT_EQ(ToBinary(0x4fff).size(), 4);
  EXPECT_EQ(ToBinary(0x3eff'ffff).size(), 4);
  EXPECT_EQ(ToBinary(0x4f00'0000).size(), 9);
  EXPECT_EQ(ToBinary(0xffff'ffff'ffff'ffff).size(), 9);

  EXPECT_EQ(ToBinary(-1).size(), 9);
}

TEST(CacheDumpCommon, String) {
  TestWriteReadCycle(std::string{""});
  TestWriteReadCycle(std::string{"a"});
  TestWriteReadCycle(std::string{"abc"});
  TestWriteReadCycle(std::string{"A big brown hog jumps over the lazy dog"});
}

TEST(CacheDumpCommon, Bool) {
  TestWriteReadCycle(false);
  TestWriteReadCycle(true);
}

struct TwoStrings {
  TwoStrings(std::string_view a, std::string_view b) : a(a), b(b) {}

  std::string a;
  std::string b;

  bool operator==(const TwoStrings& other) const {
    return a == other.a && b == other.b;
  }
};

void Write(dump::Writer& writer, const TwoStrings& value) {
  writer.Write(value.a.size());
  writer.Write(value.b.size());
  dump::WriteStringViewUnsafe(writer, value.a);
  dump::WriteStringViewUnsafe(writer, value.b);
}

TwoStrings Read(dump::Reader& reader, dump::To<TwoStrings>) {
  // We can't just read one `string_view` after another. We could have copied
  // the first `string_view` to a `string`, but this example is being inventive.
  const auto size_a = reader.Read<std::size_t>();
  const auto size_b = reader.Read<std::size_t>();
  const auto ab = dump::ReadStringViewUnsafe(reader, size_a + size_b);
  return TwoStrings{ab.substr(0, size_a), ab.substr(size_a, size_b)};
}

TEST(CacheDumpCommon, ReadStringViewUnsafe) {
  TestWriteReadCycle(TwoStrings{"", "abc"});
}

namespace formats::json {

void Write(dump::Writer& writer, const formats::json::Value& value) {
  // formats::json::ToString does not currently produce std::string_view, but
  // could be modified to do so.
  writer.Write(formats::json::ToString(value));
}

formats::json::Value Read(dump::Reader& reader,
                          dump::To<formats::json::Value>) {
  return formats::json::FromString(dump::ReadStringViewUnsafe(reader));
}

}  // namespace formats::json

TEST(CacheDumpCommon, StringView) {
  EXPECT_TRUE(dump::kIsWritable<std::string_view>);
  EXPECT_FALSE(dump::kIsReadable<std::string_view>);
  TestWriteReadCycle(formats::json::MakeObject("foo", 42, "bar", "baz"));
}

TEST(CacheDumpCommon, Enum) {
  enum class Dummy {
    kA = 5,
    kB = 7,
    kC = 7,
  };

  TestWriteReadCycle(Dummy{});
  TestWriteReadCycle(Dummy::kA);
  TestWriteReadCycle(Dummy::kB);
  TestWriteReadCycle(Dummy::kC);
}

TEST(CacheDumpCommon, Duration) {
  TestWriteReadCycle(std::chrono::nanoseconds{5});
  TestWriteReadCycle(std::chrono::microseconds{0});
  TestWriteReadCycle(std::chrono::milliseconds{-5});
  TestWriteReadCycle(std::chrono::seconds{5});
  TestWriteReadCycle(std::chrono::minutes{9'000'000});
  TestWriteReadCycle(std::chrono::hours{5});

  TestWriteReadCycle(std::chrono::system_clock::duration{42});
  TestWriteReadCycle(std::chrono::steady_clock::duration{42});

  TestWriteReadCycle(std::chrono::duration<int8_t, std::ratio<2, 7>>{42});
}

TEST(CacheDumpCommon, TimePoint) {
  TestWriteReadCycle(std::chrono::system_clock::time_point{} +
                     std::chrono::minutes{5});

  TestWriteReadCycle(std::chrono::time_point<
                         std::chrono::system_clock,
                         std::chrono::duration<int8_t, std::ratio<2, 7>>>{} +
                     std::chrono::minutes{5});

  EXPECT_FALSE(dump::kIsDumpable<std::chrono::steady_clock::time_point>);
}

TEST(CacheDumpCommon, UUID) {
  boost::uuids::random_generator gen;
  for (int i = 0; i < 1000; ++i) {
    TestWriteReadCycle(gen());
  }
}

TEST(CacheDumpCommon, ReadEntire) {
  for (int pre_read = 0; pre_read <= 1; ++pre_read) {
    for (int exp_size = 0; exp_size < 25; ++exp_size) {
      std::string data(1 << exp_size, 'a');

      dump::MockReader reader(data);
      if (pre_read) dump::ReadStringViewUnsafe(reader, 1);

      const auto result = dump::ReadEntire(reader);
      EXPECT_EQ(result, data.substr(pre_read));
    }
  }
}

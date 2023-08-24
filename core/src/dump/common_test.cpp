#include <userver/dump/common.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include <boost/uuid/random_generator.hpp>

#include <userver/decimal64/decimal64.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/rand.hpp>

#include <userver/dump/operations_mock.hpp>
#include <userver/dump/test_helpers.hpp>

USERVER_NAMESPACE_BEGIN

using dump::TestWriteReadCycle;

template <typename T>
class DumpCommonNumeric : public testing::Test {
 public:
  using Num = T;
};

using NumericTestTypes =
    ::testing::Types<int, std::uint8_t, std::int8_t, std::uint16_t,
                     std::int16_t, std::uint32_t, std::int32_t, std::uint64_t,
                     std::int64_t, float, double, long double>;
TYPED_TEST_SUITE(DumpCommonNumeric, NumericTestTypes);

TYPED_TEST(DumpCommonNumeric, Numeric) {
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
    auto& engine = utils::DefaultRandom();
    std::uniform_int_distribution<Num> distribution(
        std::numeric_limits<Num>::min(), std::numeric_limits<Num>::max());
    for (int i = 0; i < 1000; ++i) {
      TestWriteReadCycle(distribution(engine));
    }
  }
}

TEST(DumpCommon, IntegerSizes) {
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

TEST(DumpCommon, String) {
  TestWriteReadCycle(std::string{""});
  TestWriteReadCycle(std::string{"a"});
  TestWriteReadCycle(std::string{"abc"});
  TestWriteReadCycle(std::string{"A big brown hog jumps over the lazy dog"});
}

TEST(DumpCommon, Bool) {
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

TEST(DumpCommon, ReadStringViewUnsafe) {
  TestWriteReadCycle(TwoStrings{"", "abc"});
}

namespace {

class MyBigInt final {
 public:
  explicit MyBigInt(std::string_view str) : data_(str) {}

  // This could be some buffer from an external serialization library.
  std::string_view AsStringView() const { return data_; }

  bool operator==(const MyBigInt& other) const { return data_ == other.data_; }

 private:
  std::string data_;
};

void Write(dump::Writer& writer, const MyBigInt& value) {
  writer.Write(value.AsStringView());
}

MyBigInt Read(dump::Reader& reader, dump::To<MyBigInt>) {
  return MyBigInt(dump::ReadStringViewUnsafe(reader));
}

}  // namespace

TEST(DumpCommon, StringView) {
  EXPECT_TRUE(dump::kIsWritable<std::string_view>);
  EXPECT_FALSE(dump::kIsReadable<std::string_view>);
  TestWriteReadCycle(MyBigInt("9001"));
}

TEST(DumpCommon, Enum) {
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

TEST(DumpCommon, Duration) {
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

TEST(DumpCommon, TimePoint) {
  TestWriteReadCycle(std::chrono::system_clock::time_point{} +
                     std::chrono::minutes{5});

  TestWriteReadCycle(std::chrono::time_point<
                         std::chrono::system_clock,
                         std::chrono::duration<int8_t, std::ratio<2, 7>>>{} +
                     std::chrono::minutes{5});

  EXPECT_FALSE(dump::kIsDumpable<std::chrono::steady_clock::time_point>);
}

TEST(DumpCommon, Decimal) {
  TestWriteReadCycle(decimal64::Decimal<2>{0});
  TestWriteReadCycle(decimal64::Decimal<3>{"0.42"});
  TestWriteReadCycle(
      decimal64::Decimal<4, decimal64::FloorRoundPolicy>{"0.4246"});
  TestWriteReadCycle(
      decimal64::Decimal<4, decimal64::CeilingRoundPolicy>{"0.4246"});
}

TEST(DumpCommon, UUID) {
  boost::uuids::random_generator gen;
  for (int i = 0; i < 1000; ++i) {
    TestWriteReadCycle(gen());
  }
}

TEST(DumpCommon, JsonValue) {
  TestWriteReadCycle(formats::json::MakeObject("foo", 42, "bar", "baz"));
}

TEST(DumpCommon, ReadEntire) {
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

USERVER_NAMESPACE_END

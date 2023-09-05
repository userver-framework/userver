#include <userver/storages/postgres/io/bitstring.hpp>

#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

enum class TestFlags {
  kNone = 0,
  k1 = 0x1,
  k2 = 0x2,
  k4 = 0x4,
  k8 = 0x8,
  k16 = 0x10,
  k32 = 0x20,
  k64 = 0x40,
  k128 = 0x80
};

constexpr std::size_t kHeaderSize =
    sizeof(int32_t);  // sizeof header with count of bits

constexpr std::uint8_t kUint8Value = 0xF2u;
constexpr std::uint64_t kUint64Value = 0xF000000000000002u;

constexpr utils::Flags<TestFlags> kFlagsValue{TestFlags::k2, TestFlags::k16,
                                              TestFlags::k32, TestFlags::k64,
                                              TestFlags::k128};

constexpr std::bitset<1> kBitset1Value{0x1};
constexpr std::bitset<8> kBitset8Value{0xF2};
constexpr std::bitset<9> kBitset9Value{0x1F2};
constexpr std::bitset<64> kBitset64Value{0xF000000000000002};

std::bitset<65> Bitset65Value() {
  return std::bitset<65>{
      "1"
      "11110000"
      "00000000"
      "00000000"
      "00000000"
      "00000000"
      "00000000"
      "00000000"
      "00000010"};
}

constexpr std::array<bool, 8> kArrayValue{false, true, false, false,
                                          true,  true, true,  true};

const pg::UserTypes types;

}  // namespace

namespace static_test {

static_assert(!tt::kIsBitStringCompatible<std::vector<bool>>);
static_assert(tt::kIsBitStringCompatible<utils::Flags<TestFlags>>);
static_assert(tt::kIsBitStringCompatible<std::array<bool, 1>> &&
              tt::kIsBitStringCompatible<std::array<bool, 128>>);
static_assert(!tt::kIsBitStringCompatible<std::array<char, 1>> &&
              !tt::kIsBitStringCompatible<std::array<char, 128>>);
static_assert(tt::kIsBitStringCompatible<std::bitset<1>> &&
              tt::kIsBitStringCompatible<std::bitset<128>>);
static_assert(tt::kIsBitStringCompatible<std::uint8_t> &&
              tt::kIsBitStringCompatible<std::uint16_t> &&
              tt::kIsBitStringCompatible<std::uint32_t> &&
              tt::kIsBitStringCompatible<std::uint64_t>);

}  // namespace static_test

namespace {

template <typename BitContainer>
std::size_t ByteCount(const BitContainer&) {
  constexpr std::size_t byte_size = 8;
  const std::size_t bit_count =
      io::traits::BitContainerTraits<BitContainer>::BitCount();
  return (bit_count + byte_size - 1) / byte_size;
}

template <typename Enum>
std::size_t ByteCount(const utils::Flags<Enum>& value) {
  return ByteCount(value.GetValue());
}

template <typename T>
void BitStringToBitMappingTest(const T& val) {
  EXPECT_EQ(io::CppToPg<decltype(pg::Bit(val))>::GetOid(types),
            static_cast<unsigned int>(
                io::PredefinedOid<io::PredefinedOids::kBit>::value));
}

template <typename T>
void BitStringToVarbitMappingTest(const T& val) {
  EXPECT_EQ(io::CppToPg<decltype(pg::Varbit(val))>::GetOid(types),
            static_cast<unsigned int>(
                io::PredefinedOid<io::PredefinedOids::kVarbit>::value));
}

template <std::size_t N>
void BitsetDefaultMappingTest(const std::bitset<N>&) {
  EXPECT_EQ(io::CppToPg<std::bitset<N>>::GetOid(types),
            static_cast<unsigned int>(
                io::PredefinedOid<io::PredefinedOids::kVarbit>::value));
}

TEST(PostgreIO, BitStringMapping) {
  BitStringToBitMappingTest(kUint8Value);
  BitStringToBitMappingTest(kFlagsValue);
  BitStringToBitMappingTest(kArrayValue);
  BitStringToBitMappingTest(kBitset1Value);

  BitStringToVarbitMappingTest(kUint8Value);
  BitStringToVarbitMappingTest(kFlagsValue);
  BitStringToVarbitMappingTest(kArrayValue);
  BitStringToVarbitMappingTest(kBitset1Value);

  BitsetDefaultMappingTest(kBitset1Value);
}

template <pg::BitStringType kBitStringType, typename T>
void BitStringIoTest(const T& src_value) {
  pg::test::Buffer buffer;
  UEXPECT_NO_THROW(
      io::WriteBuffer(types, buffer, pg::BitString<kBitStringType>(src_value)));
  EXPECT_EQ(kHeaderSize + ByteCount(src_value), buffer.size());
  auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
  T tgt_value{};
  UEXPECT_NO_THROW(
      io::ReadBuffer(fb, pg::BitString<kBitStringType>(tgt_value)));
  EXPECT_EQ(src_value, tgt_value);
}

// default bitset buffer formatter/buffer parser test
template <std::size_t N>
void BitsetIoTest(const std::bitset<N>& src_value) {
  pg::test::Buffer buffer;
  UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, src_value));
  EXPECT_EQ(kHeaderSize + ByteCount(src_value), buffer.size());
  auto fb = pg::test::MakeFieldBuffer(buffer, io::BufferCategory::kPlainBuffer);
  std::bitset<N> tgt_value;
  UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt_value));
  EXPECT_EQ(src_value, tgt_value);
}

TEST(PostgreIO, BitStringBufferIO) {
  BitStringIoTest<pg::BitStringType::kBitVarying>(kUint8Value);
  BitStringIoTest<pg::BitStringType::kBitVarying>(kUint64Value);
  BitStringIoTest<pg::BitStringType::kBitVarying>(kFlagsValue);
  BitStringIoTest<pg::BitStringType::kBitVarying>(kArrayValue);
  BitStringIoTest<pg::BitStringType::kBitVarying>(kBitset1Value);
  BitStringIoTest<pg::BitStringType::kBitVarying>(kBitset8Value);
  BitStringIoTest<pg::BitStringType::kBitVarying>(Bitset65Value());

  BitStringIoTest<pg::BitStringType::kBit>(kUint8Value);
  BitStringIoTest<pg::BitStringType::kBit>(kUint64Value);
  BitStringIoTest<pg::BitStringType::kBit>(kFlagsValue);
  BitStringIoTest<pg::BitStringType::kBit>(kArrayValue);
  BitStringIoTest<pg::BitStringType::kBit>(kBitset1Value);
  BitStringIoTest<pg::BitStringType::kBit>(kBitset8Value);
  BitStringIoTest<pg::BitStringType::kBit>(Bitset65Value());

  BitsetIoTest(kBitset1Value);
  BitsetIoTest(kBitset8Value);
  BitsetIoTest(Bitset65Value());
}

template <pg::BitStringType kBitStringType, typename T, typename Connection>
void BitStringRoundtripTest(const T& src_value, const Connection& conn) {
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = conn->Execute(
                       "select $1", pg::BitString<kBitStringType>(src_value)));
  T tgt_value{};
  UEXPECT_NO_THROW(res[0][0].To(pg::BitString<kBitStringType>(tgt_value)));
  EXPECT_EQ(src_value, tgt_value);
}

// default bitset buffer formatter/buffer parser test
template <std::size_t N, typename Connection>
void BitsetRoundtripTest(const std::bitset<N>& src_value,
                         const Connection& conn) {
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = conn->Execute("select $1", src_value));
  std::bitset<N> tgt_value;
  UEXPECT_NO_THROW(res[0][0].To(tgt_value));
  EXPECT_EQ(src_value, tgt_value);
}

UTEST_P(PostgreConnection, BitstringRoundtrip) {
  CheckConnection(GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBitVarying>(kUint8Value,
                                                         GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBitVarying>(kUint64Value,
                                                         GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBitVarying>(kFlagsValue,
                                                         GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBitVarying>(kArrayValue,
                                                         GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBitVarying>(kBitset1Value,
                                                         GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBitVarying>(kBitset8Value,
                                                         GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBitVarying>(Bitset65Value(),
                                                         GetConn());

  BitStringRoundtripTest<pg::BitStringType::kBit>(kUint8Value, GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBit>(kUint64Value, GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBit>(kFlagsValue, GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBit>(kArrayValue, GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBit>(kBitset1Value, GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBit>(kBitset8Value, GetConn());
  BitStringRoundtripTest<pg::BitStringType::kBit>(Bitset65Value(), GetConn());

  BitsetRoundtripTest(kBitset1Value, GetConn());
  BitsetRoundtripTest(kBitset8Value, GetConn());
  BitsetRoundtripTest(Bitset65Value(), GetConn());
}

template <pg::BitStringType kBitStringType, typename T, typename Connection>
void BitStringOwningRoundtripTest(const T& initial_value,
                                  const Connection& conn) {
  pg::ResultSet res{nullptr};
  const pg::BitStringWrapper<T, kBitStringType> wrapped{initial_value};
  UEXPECT_NO_THROW(res = conn->Execute("select $1", wrapped));
  pg::BitStringWrapper<T, kBitStringType> returned;
  res[0][0].To(returned);
  UEXPECT_NO_THROW(res[0][0].To(returned));
  EXPECT_EQ(wrapped.bits, returned.bits);
}

UTEST_P(PostgreConnection, BitStringOwningRoundtrip) {
  CheckConnection(GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBitVarying>(kUint8Value,
                                                               GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBitVarying>(kUint64Value,
                                                               GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBitVarying>(kFlagsValue,
                                                               GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBitVarying>(kArrayValue,
                                                               GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBitVarying>(kBitset1Value,
                                                               GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBitVarying>(kBitset8Value,
                                                               GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBitVarying>(Bitset65Value(),
                                                               GetConn());

  BitStringOwningRoundtripTest<pg::BitStringType::kBit>(kUint8Value, GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBit>(kUint64Value,
                                                        GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBit>(kFlagsValue, GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBit>(kArrayValue, GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBit>(kBitset1Value,
                                                        GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBit>(kBitset8Value,
                                                        GetConn());
  BitStringOwningRoundtripTest<pg::BitStringType::kBit>(Bitset65Value(),
                                                        GetConn());
}

template <pg::BitStringType kBitStringType, typename T, typename Connection>
void BitStringStoredTest(const T& src_value, const Connection& conn) {
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = conn->Execute("select $1",
                          pg::ParameterStore{}.PushBack(
                              pg::BitString<kBitStringType>(src_value))));
  T tgt_value{};
  UEXPECT_NO_THROW(res[0][0].To(pg::BitString<kBitStringType>(tgt_value)));
  EXPECT_EQ(src_value, tgt_value);
}

UTEST_P(PostgreConnection, BitStringStored) {
  CheckConnection(GetConn());
  BitStringStoredTest<pg::BitStringType::kBitVarying>(kUint8Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBitVarying>(kUint64Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBitVarying>(kFlagsValue, GetConn());
  BitStringStoredTest<pg::BitStringType::kBitVarying>(kArrayValue, GetConn());
  BitStringStoredTest<pg::BitStringType::kBitVarying>(kBitset1Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBitVarying>(kBitset8Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBitVarying>(Bitset65Value(),
                                                      GetConn());

  BitStringStoredTest<pg::BitStringType::kBit>(kUint8Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBit>(kUint64Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBit>(kFlagsValue, GetConn());
  BitStringStoredTest<pg::BitStringType::kBit>(kArrayValue, GetConn());
  BitStringStoredTest<pg::BitStringType::kBit>(kBitset1Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBit>(kBitset8Value, GetConn());
  BitStringStoredTest<pg::BitStringType::kBit>(Bitset65Value(), GetConn());
}

template <pg::BitStringType kBitStringType, typename T, typename U,
          typename Connection>
void BitStringNoOverflowTest(const U& src_value, const T& expected_value,
                             const Connection& conn) {
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = conn->Execute(
                       "select $1", pg::BitString<kBitStringType>(src_value)));
  T tgt_value{};
  UEXPECT_NO_THROW(res[0][0].To(pg::BitString<kBitStringType>(tgt_value)));
  EXPECT_EQ(expected_value, tgt_value);
}

template <pg::BitStringType kBitStringType, typename T, typename U,
          typename Connection>
void BitStringOverflowTest(const U& src_value, const Connection& conn) {
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = conn->Execute(
                       "select $1", pg::BitString<kBitStringType>(src_value)));
  T tgt_value{};
  UEXPECT_THROW(res[0][0].To(pg::BitString<kBitStringType>(tgt_value)),
                USERVER_NAMESPACE::storages::postgres::BitStringOverflow);
}

UTEST_P(PostgreConnection, BitStringOverflow) {
  CheckConnection(GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBitVarying>(
      kBitset8Value, kUint8Value, GetConn());
  BitStringOverflowTest<pg::BitStringType::kBitVarying, std::uint8_t>(
      kBitset9Value, GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBitVarying>(
      kBitset64Value, kUint64Value, GetConn());
  BitStringOverflowTest<pg::BitStringType::kBitVarying, std::uint64_t>(
      Bitset65Value(), GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBitVarying>(
      kBitset8Value, kFlagsValue, GetConn());
  BitStringOverflowTest<pg::BitStringType::kBitVarying,
                        std::decay_t<decltype(kFlagsValue)>>(Bitset65Value(),
                                                             GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBitVarying>(
      kBitset8Value, kArrayValue, GetConn());
  BitStringOverflowTest<pg::BitStringType::kBitVarying,
                        std::decay_t<decltype(kArrayValue)>>(kBitset9Value,
                                                             GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBitVarying>(
      kBitset8Value, kBitset8Value, GetConn());
  BitStringOverflowTest<pg::BitStringType::kBitVarying,
                        std::decay_t<decltype(kBitset8Value)>>(kBitset9Value,
                                                               GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBit>(kBitset8Value, kUint8Value,
                                                   GetConn());
  BitStringOverflowTest<pg::BitStringType::kBit, std::uint8_t>(kBitset9Value,
                                                               GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBit>(kBitset64Value, kUint64Value,
                                                   GetConn());
  BitStringOverflowTest<pg::BitStringType::kBit, std::uint64_t>(Bitset65Value(),
                                                                GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBit>(kBitset64Value, kUint64Value,
                                                   GetConn());
  BitStringOverflowTest<pg::BitStringType::kBit, std::uint64_t>(Bitset65Value(),
                                                                GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBit>(kBitset8Value, kArrayValue,
                                                   GetConn());
  BitStringOverflowTest<pg::BitStringType::kBit,
                        std::decay_t<decltype(kArrayValue)>>(kBitset9Value,
                                                             GetConn());

  BitStringNoOverflowTest<pg::BitStringType::kBit>(kBitset8Value, kBitset8Value,
                                                   GetConn());
  BitStringOverflowTest<pg::BitStringType::kBit,
                        std::decay_t<decltype(kBitset8Value)>>(kBitset9Value,
                                                               GetConn());
}

UTEST_P(PostgreConnection, BitStringSample) {
  pg::ResultSet res{nullptr};
  auto& conn = GetConn();
  CheckConnection(conn);

  /*! [Bit string sample] */
  res = conn->Execute("select 25::bit(8)");
  auto bits = res[0][0].As<std::bitset<8>>();
  EXPECT_EQ(bits, 0b11001);

  res = conn->Execute("select $1::text", bits);
  EXPECT_EQ(res[0][0].As<std::string>(), "00011001");

  std::uint8_t num{};
  res = conn->Execute("select 42::bit(8)");
  res[0][0].To(pg::Bit(num));
  EXPECT_EQ(num, 42);
  /*! [Bit string sample] */
}

}  // namespace

USERVER_NAMESPACE_END

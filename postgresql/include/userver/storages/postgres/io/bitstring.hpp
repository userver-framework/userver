#pragma once

/// @file userver/storages/postgres/io/bitstring.hpp
/// @brief storages::postgres::BitString I/O support
/// @ingroup userver_postgres_parse_and_format

#include <bitset>
#include <vector>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/**
 * @page pg_bitstring uPg 'bit' and 'bit varying' support
 *
 * The driver allows reading and writing raw binary data from/to PostgreSQL
 * 'bit' and 'bit varying' types.
 *
 * Reading and writing to PostgreSQL is implemented for integral values
 * (e.g. int8_t, int32_t, int64_t), 'utils::Flags',
 * 'std::array' of 'bool' and 'std::bitset'.
 *
 * @code
 * namespace pg = storages::postgres;
 * std::array<bool, 4> arr = "1101";
 * trx.Execute("select $1", pg::Varbit(arr));
 * trx.Execute("select $1", pg::Bit(arr));
 *
 * // std::bitset mapps to bit varying on default
 * std::bitset<4> bits = "1101";
 * trx.Execute("select $1", bits);
 * @endcode
 */

namespace io::traits {

template <typename BitContainer>
struct IsBitStringCompatible : std::is_integral<BitContainer> {};

template <typename Enum>
struct IsBitStringCompatible<USERVER_NAMESPACE::utils::Flags<Enum>>
    : std::true_type {};

template <std::size_t N>
struct IsBitStringCompatible<std::bitset<N>> : std::true_type {};

template <std::size_t N>
struct IsBitStringCompatible<std::array<bool, N>> : std::true_type {};

template <typename T>
inline constexpr bool kIsBitStringCompatible = IsBitStringCompatible<T>::value;

template <typename BitContainer, typename Enable = void>
struct BitContainerTraits;

template <typename BitContainer>
struct BitContainerTraits<BitContainer,
                          std::enable_if_t<std::is_integral_v<BitContainer>>> {
  static bool TestBit(const BitContainer& bits, std::uint8_t i) {
    return bits & (1ull << i);
  }
  static void SetBit(BitContainer& bits, std::uint8_t i) {
    bits |= (1ull << i);
  }
  static std::size_t BitCount(const BitContainer&) noexcept {
    return sizeof(BitContainer) * 8;
  }
  static void Reset(BitContainer& bits) noexcept { bits = 0; }
};

template <std::size_t N>
struct BitContainerTraits<std::array<bool, N>> {
  using BitContainer = std::array<bool, N>;
  static bool TestBit(const BitContainer& bits, std::uint8_t i) {
    return bits[i];
  }
  static void SetBit(BitContainer& bits, std::uint8_t i) { bits[i] = true; }
  static std::size_t BitCount(const BitContainer&) noexcept { return N; }
  static void Reset(BitContainer& bits) noexcept {
    std::fill(bits.begin(), bits.end(), false);
  }
};

template <std::size_t N>
struct BitContainerTraits<std::bitset<N>> {
  using BitContainer = std::bitset<N>;
  static bool TestBit(const BitContainer& bits, std::uint8_t i) {
    return bits.test(i);
  }
  static void SetBit(BitContainer& bits, std::uint8_t i) { bits.set(i); }
  static std::size_t BitCount(const BitContainer&) noexcept { return N; }
  static void Reset(BitContainer& bits) noexcept { bits.reset(); }
};

}  // namespace io::traits

enum class BitStringType { kBit, kBitVarying };

namespace detail {

enum class BitContainerInterface { kCommon, kFlags };

template <typename BitContainerRef, BitContainerInterface, BitStringType>
struct BitStringRefWrapper {
  static_assert(std::is_reference<BitContainerRef>::value,
                "The container must be passed by reference");

  using BitContainer = std::decay_t<BitContainerRef>;
  static_assert(io::traits::kIsBitStringCompatible<BitContainer>,
                "This C++ type cannot be used with PostgreSQL `bit` and 'bit "
                "varying' data type");

  BitContainerRef bits;
};

}  // namespace detail

template <typename BitContainer, BitStringType>
struct BitStringWrapper {
  static_assert(!std::is_reference<BitContainer>::value,
                "The container must not be passed by reference");

  static_assert(io::traits::kIsBitStringCompatible<BitContainer>,
                "This C++ type cannot be used with PostgreSQL 'bit' and `bit "
                "varying` data type");

  BitContainer bits{};
};

template <BitStringType kBitStringType, typename BitContainer>
constexpr detail::BitStringRefWrapper<
    const BitContainer&, detail::BitContainerInterface::kCommon, kBitStringType>
BitString(const BitContainer& bits) {
  return {bits};
}

template <BitStringType kBitStringType, typename BitContainer>
constexpr detail::BitStringRefWrapper<
    BitContainer&, detail::BitContainerInterface::kCommon, kBitStringType>
BitString(BitContainer& bits) {
  return {bits};
}

template <BitStringType kBitStringType, typename Enum>
constexpr detail::BitStringRefWrapper<
    const USERVER_NAMESPACE::utils::Flags<Enum>&,
    detail::BitContainerInterface::kFlags, kBitStringType>
BitString(const USERVER_NAMESPACE::utils::Flags<Enum>& bits) {
  return {bits};
}

template <BitStringType kBitStringType, typename Enum>
constexpr detail::BitStringRefWrapper<USERVER_NAMESPACE::utils::Flags<Enum>&,
                                      detail::BitContainerInterface::kFlags,
                                      kBitStringType>
BitString(USERVER_NAMESPACE::utils::Flags<Enum>& bits) {
  return {bits};
}

template <typename BitContainer>
constexpr auto Varbit(BitContainer&& bits) {
  return BitString<BitStringType::kBitVarying>(
      std::forward<BitContainer>(bits));
}

template <typename BitContainer>
constexpr auto Bit(BitContainer&& bits) {
  return BitString<BitStringType::kBit>(std::forward<BitContainer>(bits));
}

namespace io {

template <typename BitContainerRef, BitStringType kBitStringType>
struct BufferParser<postgres::detail::BitStringRefWrapper<
    BitContainerRef, postgres::detail::BitContainerInterface::kCommon,
    kBitStringType>>
    : detail::BufferParserBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kCommon,
          kBitStringType>&&> {
  using BitContainer = std::decay_t<BitContainerRef>;
  using BaseType =
      detail::BufferParserBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kCommon,
          kBitStringType>&&>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    Integer bit_count{0};
    buffer.Read(bit_count, BufferCategory::kPlainBuffer);
    if (static_cast<std::size_t>((bit_count + 7) / 8) > buffer.length)
      throw InvalidBitStringRepresentation{};

    auto& bits = this->value.bits;
    if (const Integer target_bit_count =
            io::traits::BitContainerTraits<BitContainer>::BitCount(bits);
        target_bit_count < bit_count) {
      throw BitStringOverflow(bit_count, target_bit_count);
    }
    io::traits::BitContainerTraits<BitContainer>::Reset(bits);
    const std::uint8_t* byte_cptr = buffer.buffer;
    for (Integer i = 0; i < bit_count;) {
      if ((*byte_cptr) & (0x80 >> (i % 8))) {
        io::traits::BitContainerTraits<BitContainer>::SetBit(bits, i);
      }
      ++i;
      if (i % 8 == 0) {
        ++byte_cptr;
      }
    }
  }
};

template <typename BitContainerRef, BitStringType kBitStringType>
struct BufferParser<postgres::detail::BitStringRefWrapper<
    BitContainerRef, postgres::detail::BitContainerInterface::kFlags,
    kBitStringType>>
    : detail::BufferParserBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kFlags,
          kBitStringType>&&> {
  using BitContainer = std::decay_t<BitContainerRef>;
  using BaseType =
      detail::BufferParserBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kFlags,
          kBitStringType>&&>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    typename BitContainer::ValueType bits{0};
    ReadBuffer(buffer, BitString<kBitStringType>(bits));
    this->value.bits.SetValue(bits);
  }
};

template <typename BitContainer, BitStringType kBitStringType>
struct BufferParser<postgres::BitStringWrapper<BitContainer, kBitStringType>>
    : detail::BufferParserBase<
          postgres::BitStringWrapper<BitContainer, kBitStringType>> {
  using BaseType = detail::BufferParserBase<
      postgres::BitStringWrapper<BitContainer, kBitStringType>>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    ReadBuffer(buffer, BitString<kBitStringType>(this->value.bits));
  }
};

template <std::size_t N>
struct BufferParser<std::bitset<N>> : detail::BufferParserBase<std::bitset<N>> {
  using BaseType = detail::BufferParserBase<std::bitset<N>>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    ReadBuffer(buffer, Varbit(this->value));
  }
};

template <typename BitContainerRef, BitStringType kBitStringType>
struct BufferFormatter<postgres::detail::BitStringRefWrapper<
    BitContainerRef, postgres::detail::BitContainerInterface::kCommon,
    kBitStringType>>
    : detail::BufferFormatterBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kCommon,
          kBitStringType>> {
  using BitContainer = std::decay_t<BitContainerRef>;
  using BaseType =
      detail::BufferFormatterBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kCommon,
          kBitStringType>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    // source bitcontainer (bits from least to most significant):
    //  [b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 b10 b11 b12 b13 b14 b15]
    // target buffer:
    //  header:
    //   | bit count (integer, 4 bytes in network bytes order) |
    //  bytes (bits from most to least significant):
    //   | 1st byte (b0 b1 b2 b3 b4 b5 b6 b7) |
    //   | 2nd byte (b8 b9 b10 b11 b12 b13 b14 b15) |
    // ...

    std::uint8_t b = 0;
    std::string data;
    const auto& bits = this->value.bits;
    const auto bit_count =
        io::traits::BitContainerTraits<BitContainer>::BitCount(bits);
    for (std::size_t i = 0; i < bit_count;) {
      if (io::traits::BitContainerTraits<BitContainer>::TestBit(bits, i)) {
        b |= (0x80 >> (i % 8));
      }
      ++i;
      if (i % 8 == 0) {
        data.push_back(b);
        b = 0;
      }
    }
    // push back last partially filled byte
    if (bit_count % 8) {
      data.push_back(b);
    }

    buffer.reserve(buffer.size() + sizeof(Integer) + data.size());
    WriteBuffer(types, buffer, static_cast<Integer>(bit_count));
    buffer.insert(buffer.end(), data.begin(), data.end());
  }
};

template <typename BitContainerRef, BitStringType kBitStringType>
struct BufferFormatter<postgres::detail::BitStringRefWrapper<
    BitContainerRef, postgres::detail::BitContainerInterface::kFlags,
    kBitStringType>>
    : detail::BufferFormatterBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kFlags,
          kBitStringType>> {
  using BitContainer = std::decay_t<BitContainerRef>;
  using BaseType =
      detail::BufferFormatterBase<postgres::detail::BitStringRefWrapper<
          BitContainerRef, postgres::detail::BitContainerInterface::kFlags,
          kBitStringType>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    WriteBuffer(types, buffer,
                BitString<kBitStringType>(this->value.bits.GetValue()));
  }
};

template <typename BitContainer, BitStringType kBitStringType>
struct BufferFormatter<postgres::BitStringWrapper<BitContainer, kBitStringType>>
    : detail::BufferFormatterBase<
          postgres::BitStringWrapper<BitContainer, kBitStringType>> {
  using BaseType = detail::BufferFormatterBase<
      postgres::BitStringWrapper<BitContainer, kBitStringType>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    WriteBuffer(types, buffer, BitString<kBitStringType>(this->value.bits));
  }
};

// std::bitset is saved as bit varying on default

template <std::size_t N>
struct BufferFormatter<std::bitset<N>>
    : detail::BufferFormatterBase<std::bitset<N>> {
  using BitContainer = std::bitset<N>;
  using BaseType = detail::BufferFormatterBase<std::bitset<N>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    WriteBuffer(types, buffer, Varbit(this->value));
  }
};

template <typename BitContainer,
          postgres::detail::BitContainerInterface kContainerInterface>
struct CppToSystemPg<postgres::detail::BitStringRefWrapper<
    BitContainer, kContainerInterface, postgres::BitStringType::kBitVarying>>
    : PredefinedOid<PredefinedOids::kVarbit> {};
template <typename BitContainer>
struct CppToSystemPg<postgres::BitStringWrapper<
    BitContainer, postgres::BitStringType::kBitVarying>>
    : PredefinedOid<PredefinedOids::kVarbit> {};

template <typename BitContainer,
          postgres::detail::BitContainerInterface kContainerInterface>
struct CppToSystemPg<postgres::detail::BitStringRefWrapper<
    BitContainer, kContainerInterface, postgres::BitStringType::kBit>>
    : PredefinedOid<PredefinedOids::kBit> {};
template <typename BitContainer>
struct CppToSystemPg<
    postgres::BitStringWrapper<BitContainer, postgres::BitStringType::kBit>>
    : PredefinedOid<PredefinedOids::kBit> {};

template <std::size_t N>
struct CppToSystemPg<std::bitset<N>> : PredefinedOid<PredefinedOids::kVarbit> {
};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END

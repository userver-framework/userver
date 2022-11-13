#include <userver/dump/common.hpp>

#include <cstdint>

#include <boost/endian/conversion.hpp>
#include <boost/uuid/uuid.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

static_assert(
    impl::kIsDumpedAsNanoseconds<std::chrono::system_clock::duration>);
static_assert(
    impl::kIsDumpedAsNanoseconds<std::chrono::steady_clock::duration>);

static_assert(impl::kIsDumpedAsNanoseconds<std::chrono::nanoseconds>);
static_assert(impl::kIsDumpedAsNanoseconds<std::chrono::microseconds>);

namespace impl {

// Integer format
//
// 1. [0; 128)
//   - uses 1 byte
//   - trivial serialization
// 2. [128; 0x4000)
//   - uses 2 bytes
//   - two highest bits set to 10
//   - big endian, highest byte goes first
// 3. [0x4000; 0x3f00'0000)
//   - uses 4 bytes
//   - two highest bits set to 11
//   - big endian, highest byte goes first
// 4. [0x3f00'0000; UINT64_MAX]
//   - uses 9 bytes
//   - starts with 0xff byte, followed by the dumped uint64_t
//   - native endianness
//
// Notes:
// - [u]int8_t is always trivially serialized using 1 byte
// - negative integers take 9 bytes, e.g. int8_t{-1} takes 9 bytes

void WriteInteger(Writer& writer, std::uint64_t value) {
  if (value < 0x80) {
    WriteTrivial(writer, static_cast<std::uint8_t>(value));
  } else if (value < 0x4000) {
    WriteTrivial(writer, boost::endian::native_to_big(
                             static_cast<std::uint16_t>(value | 0x8000)));
  } else if (value < 0x3f00'0000) {
    WriteTrivial(writer, boost::endian::native_to_big(
                             static_cast<std::uint32_t>(value | 0xc000'0000)));
  } else {
    WriteTrivial(writer, std::uint8_t{0xff});
    WriteTrivial(writer, value);
  }
}

std::uint64_t ReadInteger(Reader& reader) {
  const auto head = ReadTrivial<std::uint8_t>(reader);
  if (head < 0x80) {
    return head;
  } else if (head < 0xc0) {
    std::uint16_t value = head & ~0x80;
    const auto rest = ReadStringViewUnsafe(reader, 1);
    rest.copy(reinterpret_cast<char*>(&value) + 1, 1);
    return boost::endian::big_to_native(value);
  } else if (head < 0xff) {
    std::uint32_t value = head & ~0xc0;
    const auto rest = ReadStringViewUnsafe(reader, 3);
    rest.copy(reinterpret_cast<char*>(&value) + 1, 3);
    return boost::endian::big_to_native(value);
  } else {
    return ReadTrivial<std::uint64_t>(reader);
  }
}

}  // namespace impl

std::string ReadEntire(Reader& reader) {
  constexpr std::size_t kBlockSize = 1024 * 1024;  // 1 MiB

  std::string result;
  result.resize(kBlockSize);

  std::size_t capacity_blocks = 1;

  for (std::size_t acquired_blocks = 0;; ++acquired_blocks) {
    UASSERT(acquired_blocks <= capacity_blocks);
    if (acquired_blocks == capacity_blocks) {
      capacity_blocks *= 2;
      result.resize(capacity_blocks * kBlockSize);
    }

    const auto next_block = ReadUnsafeAtMost(reader, kBlockSize);
    next_block.copy(result.data() + acquired_blocks * kBlockSize,
                    next_block.size());

    if (next_block.size() < kBlockSize) {
      result.resize(acquired_blocks * kBlockSize + next_block.size());
      return result;
    }
  }
}

void Write(Writer& writer, std::string_view value) {
  writer.Write(value.size());
  WriteStringViewUnsafe(writer, value);
}

void Write(Writer& writer, const std::string& value) {
  writer.Write(std::string_view{value});
}

std::string Read(Reader& reader, To<std::string>) {
  return std::string{ReadStringViewUnsafe(reader)};
}

void Write(Writer& writer, const char* value) {
  writer.Write(std::string_view{value});
}

void Write(Writer& writer, bool value) {
  writer.Write(value ? std::uint8_t{1} : std::uint8_t{0});
}

bool Read(Reader& reader, To<bool>) {
  const auto byte = reader.Read<std::uint8_t>();
  if (byte != 0 && byte != 1) {
    throw Error("The value of `bool` can't be greater than 1");
  }
  return byte != 0;
}

void Write(Writer& writer, const boost::uuids::uuid& value) {
  impl::WriteTrivial(writer, value);
}

boost::uuids::uuid Read(Reader& reader, To<boost::uuids::uuid>) {
  return impl::ReadTrivial<boost::uuids::uuid>(reader);
}

void Write(Writer& writer, const formats::json::Value& value) {
  writer.Write(formats::json::ToString(value));
}

formats::json::Value Read(Reader& reader, To<formats::json::Value>) {
  const auto string = ReadStringViewUnsafe(reader);
  return formats::json::FromString(string);
}

}  // namespace dump

USERVER_NAMESPACE_END

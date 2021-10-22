#include <userver/dump/unsafe.hpp>

#include <fmt/format.h>

#include <userver/dump/common.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

void WriteStringViewUnsafe(Writer& writer, std::string_view value) {
  writer.WriteRaw(value);
}

std::string_view ReadStringViewUnsafe(Reader& reader) {
  const auto size = reader.Read<std::size_t>();
  return ReadStringViewUnsafe(reader, size);
}

std::string_view ReadStringViewUnsafe(Reader& reader, std::size_t size) {
  const auto result = ReadUnsafeAtMost(reader, size);

  if (result.size() != size) {
    throw Error(
        fmt::format("Unexpected end-of-file while trying to read from the dump "
                    "file: requested-size={}",
                    size));
  }

  return result;
}

std::string_view ReadUnsafeAtMost(Reader& reader, std::size_t max_size) {
  const auto result = reader.ReadRaw(max_size);
  UASSERT(result.size() <= max_size);
  return result;
}

}  // namespace dump

USERVER_NAMESPACE_END

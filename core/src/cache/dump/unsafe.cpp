#include <cache/dump/unsafe.hpp>

#include <cache/dump/common.hpp>
#include <utils/assert.hpp>

namespace cache::dump {

void WriteStringViewUnsafe(Writer& writer, std::string_view value) {
  writer.WriteRaw(value);
}

std::string_view ReadStringViewUnsafe(Reader& reader) {
  const auto size = reader.Read<std::size_t>();
  return ReadStringViewUnsafe(reader, size);
}

std::string_view ReadStringViewUnsafe(Reader& reader, std::size_t size) {
  const auto result = reader.ReadRaw(size);
  UASSERT(result.size() == size);
  return result;
}

}  // namespace cache::dump

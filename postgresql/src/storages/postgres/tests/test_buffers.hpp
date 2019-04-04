#pragma once

#include <vector>

#include <storages/postgres/io/traits.hpp>

namespace storages {
namespace postgres {
namespace test {

using Buffer = std::vector<char>;

template <typename BufferType>
io::FieldBuffer MakeFieldBuffer(
    const BufferType& buffer, io::DataFormat format,
    io::BufferCategory cat = io::BufferCategory::kPlainBuffer) {
  return {false, format, cat, buffer.size(),
          reinterpret_cast<const std::uint8_t*>(buffer.data())};
}

}  // namespace test
}  // namespace postgres
}  // namespace storages

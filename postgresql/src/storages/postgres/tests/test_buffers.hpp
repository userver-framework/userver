#pragma once

#include <vector>

#include <storages/postgres/io/traits.hpp>

namespace storages {
namespace postgres {
namespace test {

using Buffer = std::vector<char>;

template <typename BufferType>
io::FieldBuffer MakeFieldBuffer(
    const BufferType& buffer,
    io::BufferCategory cat = io::BufferCategory::kPlainBuffer) {
  return {false, cat, buffer.size(),
          reinterpret_cast<const std::uint8_t*>(buffer.data())};
}

}  // namespace test
}  // namespace postgres
}  // namespace storages

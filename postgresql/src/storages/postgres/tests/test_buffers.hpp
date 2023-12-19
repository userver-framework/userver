#pragma once

#include <vector>

#include <userver/storages/postgres/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::test {

using Buffer = std::vector<char>;

template <typename BufferType>
io::FieldBuffer MakeFieldBuffer(
    const BufferType& buffer,
    io::BufferCategory cat = io::BufferCategory::kPlainBuffer) {
  return {false, cat, buffer.size(),
          reinterpret_cast<const std::uint8_t*>(buffer.data())};
}

}  // namespace storages::postgres::test

USERVER_NAMESPACE_END

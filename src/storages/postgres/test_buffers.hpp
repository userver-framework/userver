#pragma once

#include <storages/postgres/io/traits.hpp>

namespace storages {
namespace postgres {
namespace test {

using Buffer = std::vector<char>;

inline io::FieldBuffer MakeFieldBuffer(const Buffer& buffer,
                                       io::DataFormat format) {
  return {false, format, buffer.size(), buffer.data()};
}

}  // namespace test
}  // namespace postgres
}  // namespace storages

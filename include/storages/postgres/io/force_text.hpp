#pragma once

#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages {
namespace postgres {

namespace detail {

template <typename T>
struct ForceTextFormat {
  const T& value;
};

}  // namespace detail

template <typename T>
detail::ForceTextFormat<T> ForceTextFormat(const T& value) {
  return {value};
}

namespace io {

template <typename T>
struct BufferFormatter<
    postgres::detail::ForceTextFormat<T>, DataFormat::kTextDataFormat,
    typename std::enable_if<
        traits::HasFormatter<T, DataFormat::kTextDataFormat>::value>::type> {
  using ValueType = postgres::detail::ForceTextFormat<T>;
  const ValueType& value;

  BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    WriteBuffer<DataFormat::kTextDataFormat>(buf, value.value);
  }
};

template <typename T>
struct CppToPg<postgres::detail::ForceTextFormat<T>> : CppToPg<T> {};

}  // namespace io

}  // namespace postgres
}  // namespace storages

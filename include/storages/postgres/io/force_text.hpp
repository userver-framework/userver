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

namespace traits {

template <typename T>
struct IsMappedToPg<postgres::detail::ForceTextFormat<T>> : IsMappedToPg<T> {};

template <typename T>
struct IsSpecialMapping<postgres::detail::ForceTextFormat<T>> : std::true_type {
};

}  // namespace traits

template <typename T>
struct BufferFormatter<
    postgres::detail::ForceTextFormat<T>, DataFormat::kTextDataFormat,
    typename std::enable_if<
        traits::HasFormatter<T, DataFormat::kTextDataFormat>::value>::type> {
  using ValueType = postgres::detail::ForceTextFormat<T>;
  const ValueType& value;

  BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    WriteBuffer<DataFormat::kTextDataFormat>(types, buf, value.value);
  }
};

template <typename T>
struct CppToPg<postgres::detail::ForceTextFormat<T>,
               typename std::enable_if<traits::kIsMappedToPg<T>>::type>
    : CppToPg<T> {};

}  // namespace io

}  // namespace postgres
}  // namespace storages

#pragma once

#include <storages/postgres/io/nullable_traits.hpp>
#include <storages/postgres/io/traits.hpp>

#include <boost/optional.hpp>

namespace storages {
namespace postgres {
namespace io {
namespace traits {

/// @brief
template <typename T>
struct IsNullable<boost::optional<T>> : std::true_type {};

template <typename T>
struct GetSetNull<boost::optional<T>> {
  using ValueType = boost::optional<T>;
  inline static bool IsNull(const ValueType& v) { return !v.is_initialized(); }
  inline static void SetNull(ValueType& v) { ValueType().swap(v); }
};

}  // namespace traits

template <typename T, DataFormat F>
struct BufferParser<
    boost::optional<T>, F,
    typename std::enable_if<traits::HasParser<T, F>::value>::type> {
  using ValueType = boost::optional<T>;
  ValueType& value;

  BufferParser(ValueType& val) : value{val} {}

  void operator()(const FieldBuffer& buf) {
    T val;
    ReadBuffer<F>(buf, val);
    value = val;
  }
};

template <typename T, DataFormat F>
struct BufferFormatter<
    boost::optional<T>, F,
    typename std::enable_if<traits::HasFormatter<T, F>::value>::type> {
  using ValueType = boost::optional<T>;
  const ValueType& value;

  BufferFormatter(const ValueType& val) : value{val} {}

  template <typename Buffer>
  void operator()(Buffer& buf) const {
    if (value.is_initialized()) {
      WriteBuffer<F>(buf, *value);
    }  // else TODO throw exception
  }
};

}  // namespace io
}  // namespace postgres
}  // namespace storages

#pragma once

#include <storages/postgres/io/nullable_traits.hpp>
#include <storages/postgres/io/traits.hpp>

#include <boost/optional.hpp>

namespace storages::postgres::io {

template <typename T, DataFormat F>
struct BufferParser<boost::optional<T>, F,
                    std::enable_if_t<traits::kHasParser<T, F>>>
    : detail::BufferParserBase<boost::optional<T>> {
  using BaseType = detail::BufferParserBase<boost::optional<T>>;
  using ValueParser = typename traits::IO<T, F>::ParserType;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    T val;
    ValueParser{val}(buffer);
    this->value = val;
  }
};

template <typename T, DataFormat F>
struct BufferFormatter<boost::optional<T>, F,
                       std::enable_if_t<traits::kHasFormatter<T, F>>>
    : detail::BufferFormatterBase<boost::optional<T>> {
  using BaseType = detail::BufferFormatterBase<boost::optional<T>>;
  using ValueFormatter = typename traits::IO<T, F>::FormatterType;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    if (this->value.is_initialized()) {
      ValueFormatter{*this->value}(types, buffer);
    }
  }
};

template <typename T>
struct CppToPg<boost::optional<T>, std::enable_if_t<traits::kIsMappedToPg<T>>>
    : CppToPg<T> {};

namespace traits {

template <typename T>
struct IsNullable<boost::optional<T>> : std::true_type {};

template <typename T>
struct GetSetNull<boost::optional<T>> {
  using ValueType = boost::optional<T>;
  inline static bool IsNull(const ValueType& v) { return !v.is_initialized(); }
  inline static void SetNull(ValueType& v) { ValueType().swap(v); }
  inline static void SetDefault(ValueType& v) { v = T{}; }
};

template <typename T>
struct IsMappedToPg<boost::optional<T>> : IsMappedToPg<T> {};
template <typename T>
struct IsSpecialMapping<boost::optional<T>> : IsMappedToPg<T> {};

}  // namespace traits

}  // namespace storages::postgres::io

#pragma once

#include <iostream>

#include <boost/iostreams/stream.hpp>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/traits.hpp>
#include <utils/demangle.hpp>

namespace storages {
namespace postgres {
namespace io {
namespace traits {

namespace detail {
template <typename T, typename = std::__void_t<>>
struct HasInputOperator : std::false_type {};

template <typename T>
struct HasInputOperator<T,
                        std::__void_t<decltype(std::declval<std::istream&>() >>
                                               std::declval<T&>())>>
    : std::true_type {};

template <typename T>
using CustomTextParserDefined =
    CustomParserDefined<T, DataFormat::kTextDataFormat>;

template <typename T>
struct StreamTextParser {
  T& value;

  explicit StreamTextParser(T& val) : value{val} {}
  void operator()(const FieldBuffer& buffer) {
    using source_type = boost::iostreams::basic_array_source<char>;
    using stream_type = boost::iostreams::stream<source_type>;

    source_type source{buffer.buffer, buffer.length};
    stream_type is{source};
    T tmp;
    if (is >> tmp) {
      std::swap(tmp, value);
    } else {
      std::string b{buffer.buffer, buffer.length};
      throw TextParseFailure{::utils::GetTypeName(std::type_index(typeid(T))),
                             b};
    }
  }
};

}  // namespace detail

template <typename T>
struct Input<
    T, DataFormat::kTextDataFormat,
    typename std::enable_if<!detail::CustomTextParserDefined<T>::value &&
                            detail::HasInputOperator<T>::value>::type> {
  using type = detail::StreamTextParser<T>;
};

}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages

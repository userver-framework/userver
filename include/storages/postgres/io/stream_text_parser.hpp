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
template <typename T, typename = ::utils::void_t<>>
struct HasInputOperator : std::false_type {};

template <typename T>
struct HasInputOperator<
    T, ::utils::void_t<decltype(std::declval<std::istream&>() >>
                                std::declval<T&>())>> : std::true_type {};

template <typename T>
struct StreamTextParser {
  T& value;

  explicit StreamTextParser(T& val) : value{val} {}
  void operator()(const FieldBuffer& buffer) {
    using source_type = boost::iostreams::basic_array_source<char>;
    using stream_type = boost::iostreams::stream<source_type>;

    source_type source{reinterpret_cast<const char*>(buffer.buffer),
                       buffer.length};
    stream_type is{source};
    T tmp;
    if (is >> tmp) {
      std::swap(tmp, value);
    } else {
      throw TextParseFailure{::utils::GetTypeName<T>(), buffer.ToString()};
    }
  }
};

}  // namespace detail

template <typename T>
struct Input<T, DataFormat::kTextDataFormat,
             std::enable_if_t<!detail::kCustomTextParserDefined<T> &&
                              detail::HasInputOperator<T>()>> {
  using type = detail::StreamTextParser<T>;
};

}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages

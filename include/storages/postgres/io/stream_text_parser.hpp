#pragma once

#include <iosfwd>

#include <boost/iostreams/stream.hpp>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_traits.hpp>

#include <utils/demangle.hpp>

namespace storages {
namespace postgres {
namespace io {
namespace traits {

namespace detail {

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
                              HasInputOperator<T>() && IsMappedToPg<T>()>> {
  using type = detail::StreamTextParser<T>;
};

}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages

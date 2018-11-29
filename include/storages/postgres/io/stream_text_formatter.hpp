#pragma once

#include <storages/postgres/io/traits.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <iostream>

namespace storages {
namespace postgres {
namespace io {
namespace traits {

namespace detail {

template <typename T, typename = ::utils::void_t<>>
struct HasOutputOperator : std::false_type {};

template <typename T>
struct HasOutputOperator<T,
                         ::utils::void_t<decltype(std::declval<std::ostream&>()
                                                  << std::declval<T&>())>>
    : std::true_type {};

template <typename T>
struct StreamTextFormatter {
  const T& value;

  explicit StreamTextFormatter(const T& val) : value{val} {}

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    using sink_type = boost::iostreams::back_insert_device<Buffer>;
    using stream_type = boost::iostreams::stream<sink_type>;

    {
      sink_type sink{buf};
      stream_type os{sink};
      os << value;
    }
    if (buf.empty() || buf.back() != '\0') buf.push_back('\0');
  }
};

}  // namespace detail

template <typename T>
struct Output<T, DataFormat::kTextDataFormat,
              std::enable_if_t<!detail::CustomTextFormatterDefined<T>::value &&
                               detail::HasOutputOperator<T>::value>> {
  using type = detail::StreamTextFormatter<T>;
};

}  // namespace traits

}  // namespace io
}  // namespace postgres
}  // namespace storages

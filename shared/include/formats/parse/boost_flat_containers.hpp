#pragma once

/// @file formats/parse/boost_flat_containers.hpp
/// @brief Parsers and converters for boost::container::flat_* containers.

#include <formats/parse/common_containers.hpp>
#include <formats/parse/to.hpp>

#include <boost/container/container_fwd.hpp>

namespace formats::parse {

template <class Value, typename T>
boost::container::flat_set<T> Parse(const Value& value,
                                    To<boost::container::flat_set<T>>) {
  return impl::ParseArray<boost::container::flat_set<T>, T>(
      value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
boost::container::flat_map<std::string, T> Parse(
    const Value& value, To<boost::container::flat_map<std::string, T>>) {
  return impl::ParseObject<boost::container::flat_map<std::string, T>, T>(
      value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
boost::container::flat_set<T> Convert(const Value& value,
                                      To<boost::container::flat_set<T>>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseArray<boost::container::flat_set<T>, T>(
      value, &impl::ConvertToExtractor<T, Value>);
}

template <class Value, typename T>
boost::container::flat_map<std::string, T> Convert(
    const Value& value, To<boost::container::flat_map<std::string, T>>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseObject<std::map<std::string, T>, T>(
      value, &impl::ConvertToExtractor<T, Value>);
}

}  // namespace formats::parse

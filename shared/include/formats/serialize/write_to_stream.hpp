#pragma once

/// @file formats/serialize/write_to_stream.hpp
/// @brief Common WriteToStream functions for SAX serializers.

#include <map>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <utils/meta.hpp>

namespace formats::serialize {

/// An ADL helper that allows searching for `WriteToStream` functions in
/// namespace of the stream. Derive your SAX string builder from this type.
struct SaxStream {};

/// Array like types serialization
template <typename T, typename StringBuilder>
std::enable_if_t<meta::is_vector<T>::value || meta::is_array<T>::value ||
                 meta::is_set<T>::value>
WriteToStream(const T& value, StringBuilder& sw) {
  typename StringBuilder::ArrayGuard guard(sw);
  for (const auto& item : value) {
    // explicit cast for vector<bool> shenanigans
    WriteToStream(static_cast<const typename T::value_type&>(item), sw);
  }
}

/// Dict like types serialization
template <typename T, typename StringBuilder>
std::enable_if_t<meta::is_map<T>::value> WriteToStream(const T& value,
                                                       StringBuilder& sw) {
  typename StringBuilder::ObjectGuard guard(sw);
  for (const auto& [key, value] : value) {
    sw.Key(key);
    WriteToStream(value, sw);
  }
}

/// Variant serialization
template <typename... Types, typename StringBuilder>
void WriteToStream(const std::variant<Types...>& value, StringBuilder& sw) {
  return std::visit([&sw](const auto& item) { WriteToStream(item, sw); },
                    value);
}

/// std::optional serialization
template <typename T, typename StringBuilder>
void WriteToStream(const std::optional<T>& value, StringBuilder& sw) {
  if (!value) {
    sw.WriteNull();
    return;
  }

  WriteToStream(*value, sw);
}

namespace impl {

// We specialize kIsSerializeAllowedInWriteToStream to catch the usages of
// Serialize for your type. Make sure that kIsSerializeAllowedInWriteToStream is
// visible at the point of WriteToStream instantiation.
template <class T, class StringBuilder>
constexpr inline bool kIsSerializeAllowedInWriteToStream = true;

}  // namespace impl

/// Fall back to using formats::*::Serialize
///
/// The signature of this WriteToStream must remain the less specialized one, so
/// that it is not preferred over other functions.
template <typename T, typename StringBuilder>
std::enable_if_t<!meta::is_vector<T>::value && !meta::is_array<T>::value &&
                 !meta::is_set<T>::value && !meta::is_map<T>::value &&
                 !std::is_arithmetic_v<T>>
WriteToStream(const T& value, StringBuilder& sw) {
  static_assert(impl::kIsSerializeAllowedInWriteToStream<T, StringBuilder>,
                "SAX serialization fall back to Serialize call");

  using ToValue = serialize::To<typename StringBuilder::Value>;
  sw.WriteValue(Serialize(value, ToValue{}));
}

}  // namespace formats::serialize

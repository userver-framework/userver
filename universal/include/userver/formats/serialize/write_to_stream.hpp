#pragma once

/// @file userver/formats/serialize/write_to_stream.hpp
/// @brief Common WriteToStream functions for SAX serializers.
/// @ingroup userver_formats_serialize_sax

#include <map>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <userver/utils/meta.hpp>

#include <userver/formats/common/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

/// An ADL helper that allows searching for `WriteToStream` functions in
/// namespace of the stream. Derive your SAX string builder from this type.
struct SaxStream {};

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

// Array like types serialization
template <typename T, typename StringBuilder>
void WriteToStreamArray(const T& value, StringBuilder& sw) {
  typename StringBuilder::ArrayGuard guard(sw);
  for (const auto& item : value) {
    // explicit cast for vector<bool> shenanigans
    WriteToStream(static_cast<const meta::RangeValueType<T>&>(item), sw);
  }
}

// Dict like types serialization
template <typename T, typename StringBuilder>
void WriteToStreamDict(const T& value, StringBuilder& sw) {
  typename StringBuilder::ObjectGuard guard(sw);
  for (const auto& [key, value] : value) {
    sw.Key(key);
    WriteToStream(value, sw);
  }
}

}  // namespace impl

/// Handle ranges, fall back to using formats::*::Serialize
///
/// The signature of this WriteToStream must remain the less specialized one, so
/// that it is not preferred over other functions.
template <typename T, typename StringBuilder>
std::enable_if_t<!std::is_arithmetic_v<T>> WriteToStream(const T& value,
                                                         StringBuilder& sw) {
  using Value = typename StringBuilder::Value;

  if constexpr (meta::kIsMap<T>) {
    impl::WriteToStreamDict(value, sw);
  } else if constexpr (meta::kIsRange<T>) {
    static_assert(!meta::kIsRecursiveRange<T>,
                  "Trying to log a recursive range, which can be dangerous. "
                  "(boost::filesystem::path?) Please implement WriteToStream "
                  "for your type");
    impl::WriteToStreamArray(value, sw);
  } else if constexpr (common::impl::kHasSerialize<Value, T>) {
    static_assert(
        !sizeof(T) ||
            impl::kIsSerializeAllowedInWriteToStream<T, StringBuilder>,
        "SAX serialization falls back to Serialize call, which is not "
        "allowed. Please implement WriteToStream for your type");
    sw.WriteValue(Serialize(value, serialize::To<Value>{}));
  } else {
    static_assert(!sizeof(T),
                  "Please implement WriteToStream or Serialize for your type");
  }
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END

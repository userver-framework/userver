#pragma once

/// @file userver/dump/json_helpers.hpp
/// @brief Convenience functions to load and dump as JSON in classes derived
/// from components::CachingComponentBase.

#include <memory>
#include <string>
#include <string_view>

#include <userver/dump/common.hpp>
#include <userver/dump/operations.hpp>
#include <userver/dump/unsafe.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// @brief Convenience function to use in
/// components::CachingComponentBase::WriteContents override to dump a type in
/// a human readable JSON format.
///
/// @see @ref md_en_userver_cache_dumps
template <typename T>
void WriteJson(Writer& writer, const T& contents) {
  formats::json::StringBuilder sb;
  WriteToStream(contents, sb);
  WriteStringViewUnsafe(writer, sb.GetString());
  WriteStringViewUnsafe(writer, "\n");
}

/// @brief Convenience function to use in
/// components::CachingComponentBase::ReadContents override to load a dump in
/// a human readable JSON format.
///
/// @see @ref md_en_userver_cache_dumps
template <typename T>
std::unique_ptr<const T> ReadJson(Reader& reader) {
  return std::make_unique<const T>(
      formats::json::FromString(ReadEntire(reader)).As<T>());
}

}  // namespace dump

USERVER_NAMESPACE_END

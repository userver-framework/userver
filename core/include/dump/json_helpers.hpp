#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <cache/caching_component_base.hpp>
#include <dump/common.hpp>
#include <dump/operations.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/string_builder.hpp>
#include <formats/json/value.hpp>
#include <utils/assert.hpp>
#include <utils/clang_format_workarounds.hpp>

namespace dump {

/// @{
/// Use as:
/// @code
/// dump::WriteJson(writer, contents);        // in WriteContents
/// return dump::ReadJson<DataType>(reader);  // in ReadContents
/// @endcode
template <typename T>
void WriteJson(Writer& writer, const T& contents) {
  formats::json::StringBuilder sb;
  WriteToStream(contents, sb);
  WriteStringViewUnsafe(writer, sb.GetString());
  WriteStringViewUnsafe(writer, "\n");
}

template <typename T>
std::unique_ptr<const T> ReadJson(Reader& reader) {
  return std::make_unique<const T>(
      formats::json::FromString(ReadEntire(reader)).As<T>());
}
/// @}

// TODO TAXICOMMON-3613 remove
template <typename T>
USERVER_DEPRECATED("Use dump::ReadJson<T>(reader) instead")
std::unique_ptr<const T> ReadJson(
    Reader& reader,
    [[maybe_unused]] const components::CachingComponentBase<T>* cache) {
  return std::make_unique<const T>(
      formats::json::FromString(ReadEntire(reader)).As<T>());
}

}  // namespace dump

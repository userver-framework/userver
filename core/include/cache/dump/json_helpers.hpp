#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <cache/caching_component_base.hpp>
#include <cache/dump/common.hpp>
#include <cache/dump/operations.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/string_builder.hpp>
#include <formats/json/value.hpp>
#include <utils/assert.hpp>

namespace cache::dump {

/// @{
/// Use as:
/// @code
/// cache::dump::WriteJson(writer, contents);     // in WriteContents
/// return cache::dump::ReadJson(reader, this));  // in ReadContents
/// @endcode
template <typename T>
void WriteJson(Writer& writer, const T& contents) {
  formats::json::StringBuilder sb;
  WriteToStream(contents, sb);
  WriteStringViewUnsafe(writer, sb.GetString());
  WriteStringViewUnsafe(writer, "\n");
}

template <typename T>
std::unique_ptr<const T> ReadJson(
    Reader& reader,
    [[maybe_unused]] const components::CachingComponentBase<T>* cache) {
  return std::make_unique<const T>(
      formats::json::FromString(ReadEntire(reader)).As<T>());
}
/// @}

}  // namespace cache::dump

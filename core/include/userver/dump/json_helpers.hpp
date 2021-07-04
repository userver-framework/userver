#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <userver/dump/common.hpp>
#include <userver/dump/operations.hpp>
#include <userver/dump/unsafe.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value.hpp>

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

}  // namespace dump

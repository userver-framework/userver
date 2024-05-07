#pragma once

#include <ydb-cpp-sdk/client/value/value.h>

#include <cstdint>
#include <string>

#include <userver/ydb/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

struct UndefinedTypeCategory {};

template <typename T>
inline constexpr auto kTypeCategory = UndefinedTypeCategory{};

template <>
inline constexpr auto kTypeCategory<bool> = NYdb::EPrimitiveType::Bool;

template <>
inline constexpr auto kTypeCategory<std::int32_t> = NYdb::EPrimitiveType::Int32;

template <>
inline constexpr auto kTypeCategory<std::uint32_t> =
    NYdb::EPrimitiveType::Uint32;

template <>
inline constexpr auto kTypeCategory<std::int64_t> = NYdb::EPrimitiveType::Int64;

template <>
inline constexpr auto kTypeCategory<std::uint64_t> =
    NYdb::EPrimitiveType::Uint64;

template <>
inline constexpr auto kTypeCategory<double> = NYdb::EPrimitiveType::Double;

template <>
inline constexpr auto kTypeCategory<std::string> = NYdb::EPrimitiveType::String;

template <>
inline constexpr auto kTypeCategory<Utf8> = NYdb::EPrimitiveType::Utf8;

template <>
inline constexpr auto kTypeCategory<Timestamp> =
    NYdb::EPrimitiveType::Timestamp;

template <>
inline constexpr auto kTypeCategory<formats::json::Value> =
    NYdb::EPrimitiveType::Json;

template <>
inline constexpr auto kTypeCategory<JsonDocument> =
    NYdb::EPrimitiveType::JsonDocument;

}  // namespace ydb::impl

USERVER_NAMESPACE_END

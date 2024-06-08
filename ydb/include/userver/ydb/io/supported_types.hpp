#pragma once

/// @file userver/ydb/io/supported_types.hpp
///
/// Available primitive types:
///  * ValueType::Bool,        bool
///  * ValueType::Int32,       std::int32_t
///  * ValueType::Uint32,      std::uint32_t
///  * ValueType::Int64,       std::int64_t
///  * ValueType::Uint64,      std::uint64_t
///  * ValueType::Double,      double
///  * ValueType::String,      std::string
///  * ValueType::Utf8,        ydb::Utf8
///  * ValueType::Timestamp,   std::chrono::system_clock::time_point
///
/// Available composite types:
///  * Optional,    std::optional for primitive types
///  * List,        std::vector and non-map containers
///  * Struct,      @ref ydb::kStructMemberNames "C++ structs"

#include <userver/ydb/io/insert_row.hpp>
#include <userver/ydb/io/list.hpp>
#include <userver/ydb/io/primitives.hpp>
#include <userver/ydb/io/structs.hpp>

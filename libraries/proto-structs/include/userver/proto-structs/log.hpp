#pragma once

/// @file userver/proto-structs/log.hpp
/// @brief Logging of protobuf structs via @ref logging::LogHelper

#include <userver/logging/log_helper.hpp>
#include <userver/proto-structs/convert.hpp>
#include <userver/proto-structs/type_mapping.hpp>
#include <userver/protobuf/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// @brief Logs the protobuf struct @a obj as a debug string (see `logging::operator<<` for
/// `google::protobuf::Message`). The representation uses proto field names (not `json_name`), is truncated once it
/// grows too large, `[debug_redact = true]` fields are redacted, and it must not be parsed back or relied upon for
/// stability.
template <typename TStruct>
requires proto_structs::traits::ProtoStruct<std::remove_cvref_t<TStruct>>
logging::LogHelper& operator<<(logging::LogHelper& h, const TStruct& obj) {
    try {
        return h << proto_structs::StructToMessage(obj);
    } catch (const std::exception& ex) {
        return h << "Failed to log struct: " << ex;
    }
}

}  // namespace logging

USERVER_NAMESPACE_END

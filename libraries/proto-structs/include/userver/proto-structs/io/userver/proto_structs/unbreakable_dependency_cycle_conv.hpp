#pragma once

/// @file userver/proto-structs/io/userver/proto_structs/unbreakable_dependency_cycle_conv.hpp
/// @brief Provides read/write context class with the ability to handle.
/// @ref proto_structs::UnbreakableDependecyCycle conversion

#include <userver/proto-structs/io/userver/proto_structs/unbreakable_dependency_cycle.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TField>
UnbreakableDependencyCycle ReadProtoField(
    ReadContext&,
    To<UnbreakableDependencyCycle>,
    int /*field_number*/,
    const TField& /*message_field*/
) {
    return {};
}

template <typename TField>
void WriteProtoField(
    WriteContext&,
    const UnbreakableDependencyCycle& /*struct_field*/,
    int /*field_number*/,
    TField& /*message_field*/
) {}

template <typename TField>
void WriteProtoField(
    WriteContext&,
    UnbreakableDependencyCycle&& /*struct_field*/,
    int /*field_number*/,
    TField& /*message_field*/
) {}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END

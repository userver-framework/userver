#pragma once

#include <initializer_list>
#include <string>

#include <userver/ugrpc/protobuf_visit.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// @brief Registers multiple message types during static initialization time
void RegisterMessageTypes(std::initializer_list<std::string> type_names);

/// @brief Find all known messages
///
/// @warning This is probably not an exhaustive list!
DescriptorList GetGeneratedMessages();

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END

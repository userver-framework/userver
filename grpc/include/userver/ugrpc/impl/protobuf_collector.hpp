#pragma once

#include <initializer_list>
#include <string>
#include <vector>

namespace google::protobuf {

class Descriptor;

}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// @brief Registers multiple message types during static initialization time
void RegisterMessageTypes(std::initializer_list<std::string> type_names);

/// @brief Find all known messages
///
/// @warning This is probably not an exhaustive list!
std::vector<const google::protobuf::Descriptor*> GetGeneratedMessages();

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END

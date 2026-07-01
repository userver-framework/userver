#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace google::protobuf {

class Descriptor;

}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace protobuf::impl {

/// @brief Registers multiple message types during static initialization time
void RegisterMessageTypes(std::initializer_list<std::string> type_names);

/// @brief Find a generated message descriptor by fully-qualified name
const google::protobuf::Descriptor* FindGeneratedMessage(std::string_view name);

/// @brief Find all known messages
///
/// @warning This is probably not an exhaustive list!
std::vector<const google::protobuf::Descriptor*> GetGeneratedMessages();

}  // namespace protobuf::impl

USERVER_NAMESPACE_END

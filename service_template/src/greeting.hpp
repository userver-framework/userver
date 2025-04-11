#pragma once

#include <string>
#include <string_view>

namespace service_template {

enum class UserType { kFirstTime, kKnown };

std::string SayHelloTo(std::string_view name, UserType type);

}  // namespace service_template

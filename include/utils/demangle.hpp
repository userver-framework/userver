#pragma once

#include <string>
#include <typeindex>

namespace utils {

std::string GetTypeName(const std::type_index& type);

template <typename T>
std::string GetTypeName() {
  return GetTypeName(std::type_index(typeid(T)));
}
}  // namespace utils

#include <yandex/taxi/userver/utils/demangle.hpp>
#ifdef __GNUG__
#define CXA_DEMANGLE
#include <cxxabi.h>
#elif __clang__
#define CXA_DEMANGLE
#include <cxxabi.h>
#endif

#include <stdlib.h>

namespace utils {

std::string GetTypeName(const std::type_index& type) {
#ifdef CXA_DEMANGLE
  auto ptr = abi::__cxa_demangle(type.name(), nullptr, nullptr, nullptr);
  std::string result(ptr ? ptr : "");
  free(ptr);
  return result;
#else
  return type.name();
#endif
}

}  // namespace utils

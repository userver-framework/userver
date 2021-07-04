#include <userver/compiler/demangle.hpp>
#ifdef __GNUG__
#define CXA_DEMANGLE
#include <cxxabi.h>
#elif __clang__
#define CXA_DEMANGLE
#include <cxxabi.h>
#endif

#include <cstdlib>

namespace compiler {

std::string GetTypeName(const std::type_index& type) {
#ifdef CXA_DEMANGLE
  auto ptr = abi::__cxa_demangle(type.name(), nullptr, nullptr, nullptr);
  std::string result(ptr ? ptr : "");
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
  free(ptr);
  return result;
#else
  return type.name();
#endif
}

}  // namespace compiler

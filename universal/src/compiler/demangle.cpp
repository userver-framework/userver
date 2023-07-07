#include <userver/compiler/demangle.hpp>
#if defined(__GNUG__) || defined(__clang__)
#define CXA_DEMANGLE
#include <cxxabi.h>
#endif

#include <cstdlib>

USERVER_NAMESPACE_BEGIN

namespace compiler {

namespace impl {
void Abort() noexcept { std::abort(); }
}  // namespace impl

std::string GetTypeName(std::type_index type) {
#ifdef CXA_DEMANGLE
  auto* ptr = abi::__cxa_demangle(type.name(), nullptr, nullptr, nullptr);
  std::string result(ptr ? ptr : "");
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
  free(ptr);
  return result;
#else
  return type.name();
#endif
}

}  // namespace compiler

USERVER_NAMESPACE_END

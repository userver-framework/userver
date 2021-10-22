#include <clients/dns/wrappers.hpp>

#include <fmt/format.h>

#include <userver/clients/dns/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns::impl {

GlobalInitializer::GlobalInitializer() {
  if (int ret = ::ares_library_init(ARES_LIB_INIT_ALL)) {
    throw ResolverException(
        fmt::format("Failed to initialize c-ares: {}", ::ares_strerror(ret)));
  }
}

GlobalInitializer::~GlobalInitializer() { ::ares_library_cleanup(); }

}  // namespace clients::dns::impl

USERVER_NAMESPACE_END

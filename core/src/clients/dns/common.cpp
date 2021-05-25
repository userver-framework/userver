#include <clients/dns/common.hpp>

#include <fmt/format.h>

#include <clients/dns/exception.hpp>

namespace clients::dns {

void CheckSupportedDomain(engine::io::AddrDomain domain) {
  switch (domain) {
    case engine::io::AddrDomain::kUnspecified:
    case engine::io::AddrDomain::kInet:
    case engine::io::AddrDomain::kInet6:
      break;  // okay
    default:
      throw UnsupportedDomainException(
          fmt::format("Cannot resolve in domain {}", static_cast<int>(domain)));
  }
}

}  // namespace clients::dns

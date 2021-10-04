#include <clients/dns/helpers.hpp>

#include <algorithm>

namespace clients::dns::impl {

AddrVector FilterByDomain(const AddrVector& src,
                          engine::io::AddrDomain domain) {
  if (domain == engine::io::AddrDomain::kUnspecified) return src;

  AddrVector filtered;
  for (const auto& addr : src) {
    if (addr.Domain() == domain) {
      filtered.push_back(addr);
    }
  }
  return filtered;
}

void SortAddrs(AddrVector& addrs) {
  std::stable_partition(
      addrs.begin(), addrs.end(), [](const engine::io::Sockaddr& addr) {
        return addr.Domain() == engine::io::AddrDomain::kInet6;
      });
}

}  // namespace clients::dns::impl

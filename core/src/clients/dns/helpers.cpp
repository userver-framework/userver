#include <clients/dns/helpers.hpp>

#include <algorithm>

namespace clients::dns::impl {

void SortAddrs(AddrVector& addrs) {
  std::stable_partition(
      addrs.begin(), addrs.end(), [](const engine::io::Sockaddr& addr) {
        return addr.Domain() == engine::io::AddrDomain::kInet6;
      });
}

}  // namespace clients::dns::impl

#pragma once

#include <userver/clients/dns/common.hpp>

namespace clients::dns::impl {

void SortAddrs(AddrVector& addrs);

}  // namespace clients::dns::impl

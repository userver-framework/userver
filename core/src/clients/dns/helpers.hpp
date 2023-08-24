#pragma once

#include <userver/clients/dns/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns::impl {

void SortAddrs(AddrVector& addrs);

}  // namespace clients::dns::impl

USERVER_NAMESPACE_END

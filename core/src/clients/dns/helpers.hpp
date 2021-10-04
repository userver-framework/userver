#pragma once

#include <userver/clients/dns/common.hpp>
#include <userver/engine/io/sockaddr.hpp>

namespace clients::dns::impl {

AddrVector FilterByDomain(const AddrVector& src, engine::io::AddrDomain domain);

void SortAddrs(AddrVector& addrs);

}  // namespace clients::dns::impl

#pragma once

/// @file userver/clients/dns/common.hpp
/// @brief Common DNS client declarations

#include <boost/container/small_vector.hpp>

#include <userver/engine/io/sockaddr.hpp>

USERVER_NAMESPACE_BEGIN

/// DNS client
namespace clients::dns {

using AddrVector = boost::container::small_vector<engine::io::Sockaddr, 4>;

}  // namespace clients::dns

USERVER_NAMESPACE_END

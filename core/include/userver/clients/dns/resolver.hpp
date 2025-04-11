#pragma once

/// @file userver/clients/dns/resolver.hpp
/// @brief @copybrief clients::dns::Resolver

#include <userver/clients/dns/common.hpp>
#include <userver/clients/dns/exception.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/rate_counter.hpp>

#include <userver/static_config/dns_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

/// @ingroup userver_clients
///
/// @brief Caching DNS resolver implementation.
///
/// Usually retrieved from clients::dns::Component.
///
/// Combines file-based (/etc/hosts) name resolution with network-based one.
class Resolver {
public:
    struct LookupSourceCounters {
        utils::statistics::RateCounter file;
        utils::statistics::RateCounter cached;
        utils::statistics::RateCounter cached_stale;
        utils::statistics::RateCounter cached_failure;
        utils::statistics::RateCounter network;
        utils::statistics::RateCounter network_failure;
    };

    Resolver(engine::TaskProcessor& fs_task_processor, const ::userver::static_config::DnsClient& config);
    Resolver(const Resolver&) = delete;
    Resolver(Resolver&&) = delete;
    ~Resolver();

    /// Performs a domain name resolution.
    ///
    /// Sources are tried in the following order:
    ///  - Cached file lookup table
    ///  - Cached network resolution results
    ///  - Network name servers
    ///
    /// @throws clients::dns::NotResolvedException if none of the sources provide
    /// a result within the specified deadline.
    AddrVector Resolve(const std::string& name, engine::Deadline deadline);

    /// Returns lookup source counters.
    const LookupSourceCounters& GetLookupSourceCounters() const;

    /// Forces the reload of lookup table file. Waits until the reload is done.
    void ReloadHosts();

    /// Resets the network results cache.
    void FlushNetworkCache();

    /// Removes the specified domain name from the network results cache.
    void FlushNetworkCache(const std::string& name);

private:
    class Impl;
    constexpr static size_t kSize = 1728;
    constexpr static size_t kAlignment = 16;
    utils::FastPimpl<Impl, kSize, kAlignment> impl_;
};

}  // namespace clients::dns

USERVER_NAMESPACE_END

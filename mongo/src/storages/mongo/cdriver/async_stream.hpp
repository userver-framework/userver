#pragma once

#include <mongoc/mongoc.h>

#include <userver/clients/dns/resolver_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

void CheckAsyncStreamCompatible();

struct AsyncStreamInitiatorData {
  // If equals to nullptr, use getaddrinfo(3)
  clients::dns::Resolver* dns_resolver;

  mongoc_ssl_opt_t ssl_opt;
};

mongoc_stream_t* MakeAsyncStream(const mongoc_uri_t*, const mongoc_host_list_t*,
                                 void*, bson_error_t*) noexcept;

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END

#pragma once

#include <mongoc/mongoc.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

void CheckAsyncStreamCompatible();

mongoc_stream_t* MakeAsyncStream(const mongoc_uri_t*, const mongoc_host_list_t*,
                                 void*, bson_error_t*) noexcept;

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END

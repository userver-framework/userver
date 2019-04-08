#pragma once

#include <mongoc/mongoc.h>

namespace storages::mongo::impl {

void CheckAsyncStreamCompatible();

mongoc_stream_t* MakeAsyncStream(const mongoc_uri_t*, const mongoc_host_list_t*,
                                 void*, bson_error_t*);

}  // namespace storages::mongo::impl

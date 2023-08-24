#pragma once

#include <mongoc/mongoc.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

void LogMongocMessage(mongoc_log_level_t level, const char* domain,
                      const char* message, void*);

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END

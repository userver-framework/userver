#pragma once

#include <mongoc/mongoc.h>

namespace storages::mongo::impl::cdriver {

void LogMongocMessage(mongoc_log_level_t level, const char* domain,
                      const char* message, void*);

}  // namespace storages::mongo::impl::cdriver

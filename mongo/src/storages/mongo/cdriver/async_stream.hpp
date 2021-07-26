#pragma once

#include <mongoc/mongoc.h>

#include <userver/engine/task/task_processor_fwd.hpp>

namespace storages::mongo::impl::cdriver {

void CheckAsyncStreamCompatible();

struct AsyncStreamInitiatorData {
  engine::TaskProcessor& bg_task_processor;
  mongoc_ssl_opt_t ssl_opt{};
};

mongoc_stream_t* MakeAsyncStream(const mongoc_uri_t*, const mongoc_host_list_t*,
                                 void*, bson_error_t*) noexcept;

}  // namespace storages::mongo::impl::cdriver

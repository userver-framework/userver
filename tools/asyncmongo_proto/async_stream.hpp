#pragma once

#include <mongoc.h>

#include <engine/io/socket.hpp>

namespace asyncmongo {

#if !MONGOC_CHECK_VERSION(1, 12, 0)
#error Recheck mongoc_stream_t structure!
#endif

struct AsyncStream : mongoc_stream_t {
  AsyncStream();

  static mongoc_stream_t* Create(const mongoc_uri_t*, const mongoc_host_list_t*,
                                 void*, bson_error_t*);

  engine::io::Socket socket;
  bool timed_out;
};
}

#pragma once

#include <memory>

#include <mongoc/mongoc.h>

#include <build_config.hpp>
#include <storages/mongo_ng/logger.hpp>

namespace storages::mongo_ng::impl {

// driver cannot be reinitialized after cleanup!
class GlobalInitializer {
 public:
  GlobalInitializer() {
    mongoc_init();
    mongoc_log_set_handler(&LogMongocMessage, nullptr);
    mongoc_handshake_data_append("taxi_userver", USERVER_VERSION, nullptr);
  }

  ~GlobalInitializer() { mongoc_cleanup(); }
};

struct ClientDeleter {
  void operator()(mongoc_client_t* client) const noexcept {
    mongoc_client_destroy(client);
  }
};
using UnboundClientPtr = std::unique_ptr<mongoc_client_t, ClientDeleter>;

struct CollectionDeleter {
  void operator()(mongoc_collection_t* collection) const noexcept {
    mongoc_collection_destroy(collection);
  }
};
using CollectionPtr = std::unique_ptr<mongoc_collection_t, CollectionDeleter>;

struct CursorDeleter {
  void operator()(mongoc_cursor_t* cursor) const noexcept {
    mongoc_cursor_destroy(cursor);
  }
};
using CursorPtr = std::unique_ptr<mongoc_cursor_t, CursorDeleter>;

struct DatabaseDeleter {
  void operator()(mongoc_database_t* db) const noexcept {
    mongoc_database_destroy(db);
  }
};
using DatabasePtr = std::unique_ptr<mongoc_database_t, DatabaseDeleter>;

struct FindAndModifyOptsDeleter {
  void operator()(mongoc_find_and_modify_opts_t* opts) const noexcept {
    mongoc_find_and_modify_opts_destroy(opts);
  }
};
using FindAndModifyOptsPtr =
    std::unique_ptr<mongoc_find_and_modify_opts_t, FindAndModifyOptsDeleter>;

struct ReadConcernDeleter {
  void operator()(mongoc_read_concern_t* read_concern) const noexcept {
    mongoc_read_concern_destroy(read_concern);
  }
};
using ReadConcernPtr =
    std::unique_ptr<mongoc_read_concern_t, ReadConcernDeleter>;

struct ReadPrefsDeleter {
  void operator()(mongoc_read_prefs_t* read_prefs) const noexcept {
    mongoc_read_prefs_destroy(read_prefs);
  }
};
using ReadPrefsPtr = std::unique_ptr<mongoc_read_prefs_t, ReadPrefsDeleter>;

struct StreamDeleter {
  void operator()(mongoc_stream_t* stream) const noexcept {
    mongoc_stream_destroy(stream);
  }
};
using StreamPtr = std::unique_ptr<mongoc_stream_t, StreamDeleter>;

struct UriDeleter {
  void operator()(mongoc_uri_t* uri) const noexcept { mongoc_uri_destroy(uri); }
};
using UriPtr = std::unique_ptr<mongoc_uri_t, UriDeleter>;

struct WriteConcernDeleter {
  void operator()(mongoc_write_concern_t* write_concern) const noexcept {
    mongoc_write_concern_destroy(write_concern);
  }
};
using WriteConcernPtr =
    std::unique_ptr<mongoc_write_concern_t, WriteConcernDeleter>;

}  // namespace storages::mongo_ng::impl

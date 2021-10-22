#pragma once

#include <memory>

#include <mongoc/mongoc.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

// driver cannot be reinitialized after cleanup!
class GlobalInitializer {
 public:
  GlobalInitializer();
  ~GlobalInitializer();

  // Call when it's safe to use logger, will only log once.
  static void LogInitWarningsOnce();
};

struct BulkOperationDeleter {
  void operator()(mongoc_bulk_operation_t* bulk) const noexcept {
    mongoc_bulk_operation_destroy(bulk);
  }
};
using BulkOperationPtr =
    std::unique_ptr<mongoc_bulk_operation_t, BulkOperationDeleter>;

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

class ReadPrefsPtr {
 public:
  ReadPrefsPtr() = default;
  explicit ReadPrefsPtr(mongoc_read_mode_t);
  ~ReadPrefsPtr();

  ReadPrefsPtr(const ReadPrefsPtr&);
  ReadPrefsPtr(ReadPrefsPtr&&) noexcept;
  ReadPrefsPtr& operator=(const ReadPrefsPtr&);
  ReadPrefsPtr& operator=(ReadPrefsPtr&&) noexcept;

  explicit operator bool() const;

  const mongoc_read_prefs_t* Get() const;
  mongoc_read_prefs_t* Get();

  void Reset() noexcept;

 private:
  mongoc_read_prefs_t* read_prefs_{nullptr};
};

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

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END

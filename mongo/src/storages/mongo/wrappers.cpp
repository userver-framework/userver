#include <storages/mongo/wrappers.hpp>

#include <mongoc/mongoc.h>

#include <build_config.hpp>
#include <storages/mongo/logger.hpp>

namespace storages::mongo::impl {

GlobalInitializer::GlobalInitializer() {
  mongoc_init();
  mongoc_log_set_handler(&LogMongocMessage, nullptr);
  mongoc_handshake_data_append("taxi_userver", USERVER_VERSION, nullptr);
}

GlobalInitializer::~GlobalInitializer() { mongoc_cleanup(); }

}  // namespace storages::mongo::impl

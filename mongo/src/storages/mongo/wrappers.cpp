#include <storages/mongo/wrappers.hpp>

#include <utility>

#include <mongoc/mongoc.h>

#include <build_config.hpp>
#include <crypto/openssl.hpp>
#include <storages/mongo/logger.hpp>

namespace storages::mongo::impl {

GlobalInitializer::GlobalInitializer() {
  crypto::impl::Openssl::Init();
  mongoc_init();
  mongoc_log_set_handler(&LogMongocMessage, nullptr);
  mongoc_handshake_data_append("taxi_userver", USERVER_VERSION, nullptr);
}

GlobalInitializer::~GlobalInitializer() { mongoc_cleanup(); }

ReadPrefsPtr::ReadPrefsPtr(mongoc_read_mode_t read_mode)
    : read_prefs_(mongoc_read_prefs_new(read_mode)) {}

ReadPrefsPtr::~ReadPrefsPtr() { Reset(); }

ReadPrefsPtr::ReadPrefsPtr(const ReadPrefsPtr& other) { *this = other; }

ReadPrefsPtr::ReadPrefsPtr(ReadPrefsPtr&& other) noexcept {
  *this = std::move(other);
}

ReadPrefsPtr& ReadPrefsPtr::operator=(const ReadPrefsPtr& rhs) {
  Reset();
  read_prefs_ = mongoc_read_prefs_copy(rhs.read_prefs_);
  return *this;
}

ReadPrefsPtr& ReadPrefsPtr::operator=(ReadPrefsPtr&& rhs) noexcept {
  Reset();
  read_prefs_ = std::exchange(rhs.read_prefs_, nullptr);
  return *this;
}

ReadPrefsPtr::operator bool() const { return !!Get(); }
const mongoc_read_prefs_t* ReadPrefsPtr::Get() const { return read_prefs_; }
mongoc_read_prefs_t* ReadPrefsPtr::Get() { return read_prefs_; }

void ReadPrefsPtr::Reset() noexcept {
  if (read_prefs_) {
    mongoc_read_prefs_destroy(std::exchange(read_prefs_, nullptr));
  }
}

}  // namespace storages::mongo::impl

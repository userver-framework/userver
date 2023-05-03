#include <storages/mongo/cdriver/wrappers.hpp>

#include <unistd.h>

#include <chrono>
#include <mutex>
#include <utility>

#include <mongoc/mongoc.h>

#include <build_config.hpp>
#include <crypto/openssl.hpp>
#include <storages/mongo/cdriver/logger.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/userver_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

[[maybe_unused]] void MongocCoroFrieldlyUsleep(int64_t usec, void*) noexcept {
  UASSERT(usec >= 0);
  if (engine::current_task::IsTaskProcessorThread()) {
    // we're not sure how it'll behave with interruptible sleeps
    engine::SleepFor(std::chrono::microseconds{usec});
  } else {
    ::usleep(usec);
  }
}

bool g_is_coro_friendly_usleep_used = false;

}  // namespace

GlobalInitializer::GlobalInitializer() {
  crypto::impl::Openssl::Init();
  mongoc_log_set_handler(&LogMongocMessage, nullptr);
#if MONGOC_TAXI_PATCH_LEVEL >= 2
  mongoc_usleep_set_impl(&MongocCoroFrieldlyUsleep, nullptr);
  g_is_coro_friendly_usleep_used = true;
#endif
  mongoc_init();
  mongoc_handshake_data_append("userver", utils::GetUserverVcsRevision(),
                               nullptr);
}

GlobalInitializer::~GlobalInitializer() { mongoc_cleanup(); }

void GlobalInitializer::LogInitWarningsOnce() {
  static std::once_flag once_flag;
  std::call_once(once_flag, [] {
    if (!g_is_coro_friendly_usleep_used) {
      LOG_WARNING() << "Cannot use coro-friendly usleep in mongo driver, "
                       "link against newer mongo-c-driver to fix";
    }
  });
}

ReadPrefsPtr::ReadPrefsPtr(mongoc_read_mode_t read_mode)
    : read_prefs_(mongoc_read_prefs_new(read_mode)) {}

ReadPrefsPtr::~ReadPrefsPtr() { Reset(); }

ReadPrefsPtr::ReadPrefsPtr(const ReadPrefsPtr& other) { *this = other; }

ReadPrefsPtr::ReadPrefsPtr(ReadPrefsPtr&& other) noexcept {
  *this = std::move(other);
}

ReadPrefsPtr& ReadPrefsPtr::operator=(const ReadPrefsPtr& rhs) {
  if (this == &rhs) return *this;

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

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END

#include <storages/mongo/cdriver/wrappers.hpp>

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <utility>

#include <mongoc/mongoc.h>

#include <userver/crypto/openssl.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/userver_info.hpp>

#include <storages/mongo/cdriver/logger.hpp>
#include <storages/mongo/features.hpp>

#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
#include <storages/mongo/cdriver_experimental/thread.hpp>
#endif

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

namespace {

std::atomic<bool> mongoc_initialized{false};
bool thread_backend_requested{false};

class InitMongocRegistrator final {
public:
    InitMongocRegistrator() {
        engine::RegisterThreadStartedHook([] { static const GlobalInitializer kInitializer; });
    }
};

[[maybe_unused]] const InitMongocRegistrator init_mongoc_registrator;

}  // namespace

GlobalInitializer::GlobalInitializer() {
    UINVARIANT(!engine::current_task::IsTaskProcessorThread(), "MongoDB initialization requires a native stack");
    thread_backend_requested = utils::impl::kMongoThreadBackendExperiment.IsEnabled();
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    const cdriver_experimental::MongocGlobalLifecycleScope lifecycle;
    if (thread_backend_requested) {
        UINVARIANT(
            mongoc_set_thread_backend(&cdriver_experimental::GetThreadBackend()),
            "MongoDB thread backend must be set before mongoc_init"
        );
    }
#endif
    crypto::Openssl::Init();
    mongoc_log_set_handler(&LogMongocMessage, nullptr);
    mongoc_init();
    mongoc_handshake_data_append("userver", utils::GetUserverVcsRevision(), nullptr);
    mongoc_initialized.store(true, std::memory_order_release);
}

GlobalInitializer::~GlobalInitializer() {
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    const cdriver_experimental::MongocGlobalLifecycleScope lifecycle;
#endif
    mongoc_cleanup();
}

void GlobalInitializer::CheckInitialized() {
    UINVARIANT(mongoc_initialized.load(std::memory_order_acquire), "MongoDB must initialize before running tasks");
    if (thread_backend_requested != utils::impl::kMongoThreadBackendExperiment.IsEnabled()) {
        throw InvalidConfigException(
            "MongoDB thread backend mode differs from the process initialization mode; "
            "restart the process with a consistent components_manager.userver_experiments.mongo-thread-backend setting"
        );
    }
}

void GlobalInitializer::LogInitWarningsOnce() {
    static std::once_flag once_flag;
    std::call_once(once_flag, [] {
#ifndef USERVER_FEATURE_MONGO_EXPERIMENTAL
        if (thread_backend_requested) {
            LOG_WARNING() << "MongoDB thread backend is unavailable in this build; using native threading primitives";
        }
#endif
#if !MONGOC_CHECK_VERSION(1, 26, 0)
        LOG_WARNING()
            << "Cannot use coro-friendly usleep in mongo driver, "
               "link against newer mongo-c-driver to fix";
#endif
    });
}

ReadPrefsPtr::ReadPrefsPtr(mongoc_read_mode_t read_mode)
    : read_prefs_(mongoc_read_prefs_new(read_mode))
{}

ReadPrefsPtr::~ReadPrefsPtr() { Reset(); }

ReadPrefsPtr::ReadPrefsPtr(const ReadPrefsPtr& other) { *this = other; }

ReadPrefsPtr::ReadPrefsPtr(ReadPrefsPtr&& other) noexcept { *this = std::move(other); }

ReadPrefsPtr& ReadPrefsPtr::operator=(const ReadPrefsPtr& rhs) {
    if (this == &rhs) {
        return *this;
    }

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

ReadPrefsPtr MakeReadPrefsWithDefaultMaxStaleness(
    const ReadPrefsPtr& read_prefs,
    const std::optional<std::chrono::seconds>& default_max_staleness
) {
    if (!read_prefs) {
        return {};
    }

    ReadPrefsPtr effective_read_prefs{read_prefs};
    if (!default_max_staleness || mongoc_read_prefs_get_mode(effective_read_prefs.Get()) == MONGOC_READ_PRIMARY ||
        mongoc_read_prefs_get_max_staleness_seconds(effective_read_prefs.Get()) != MONGOC_NO_MAX_STALENESS)
    {
        return effective_read_prefs;
    }

    mongoc_read_prefs_set_max_staleness_seconds(effective_read_prefs.Get(), default_max_staleness->count());
    return effective_read_prefs;
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END

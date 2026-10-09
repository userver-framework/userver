#include <userver/storages/mongo/pool_config.hpp>

#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/features.hpp>
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
#include <storages/mongo/cdriver_experimental/thread.hpp>
// The symbol name is part of the mongo-c-driver C ABI.
// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" const void* _mongoc_mcommon_thread_backend_get(void);
#endif
#include <userver/storages/mongo/exception.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MongoBackendConfig, BackendCanBeDisabled) {
    utils::impl::UserverExperimentsScope scope;
    scope.Set(utils::impl::kMongoThreadBackendExperiment, false);
    storages::mongo::PoolConfig config;
    EXPECT_NO_THROW(config.Validate("default"));
    config.driver_impl = storages::mongo::PoolConfig::DriverImpl::kMongoCDriver;
    EXPECT_NO_THROW(config.Validate("legacy"));
    config.driver_impl = storages::mongo::PoolConfig::DriverImpl::kMongoCDriverExperimental;
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    EXPECT_NO_THROW(config.Validate("experimental"));
#else
    EXPECT_THROW(config.Validate("experimental"), storages::mongo::InvalidConfigException);
#endif
}

TEST(MongoBackendConfig, ExplicitOptIn) {
    utils::impl::UserverExperimentsScope scope;
    scope.Set(utils::impl::kMongoThreadBackendExperiment, true);
    storages::mongo::PoolConfig config;
    EXPECT_NO_THROW(config.Validate("default"));
    config.driver_impl = storages::mongo::PoolConfig::DriverImpl::kMongoCDriverExperimental;
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    EXPECT_NO_THROW(config.Validate("experimental"));
#else
    EXPECT_THROW(config.Validate("experimental"), storages::mongo::InvalidConfigException);
#endif
}

UTEST(MongoBackendConfig, BackendMatchesOptIn) {
    storages::mongo::impl::cdriver::GlobalInitializer::CheckInitialized();
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    const auto* backend = static_cast<const bson_thread_backend_t*>(_mongoc_mcommon_thread_backend_get());
    if (utils::impl::kMongoThreadBackendExperiment.IsEnabled()) {
        ASSERT_NE(backend, nullptr);
        const auto& experimental = storages::mongo::impl::cdriver_experimental::GetThreadBackend();
        EXPECT_EQ(backend->mutex_init, experimental.mutex_init);
        EXPECT_EQ(backend->thread_create, experimental.thread_create);
    } else {
        EXPECT_EQ(backend, nullptr);
    }
#endif
}

UTEST(MongoBackendConfig, CannotChangeModeAfterInitialization) {
    storages::mongo::impl::cdriver::GlobalInitializer::CheckInitialized();
    utils::impl::UserverExperimentsScope scope;
    scope.Set(utils::impl::kMongoThreadBackendExperiment, !utils::impl::kMongoThreadBackendExperiment.IsEnabled());
    UEXPECT_THROW_MSG(
        storages::mongo::impl::cdriver::GlobalInitializer::CheckInitialized(),
        storages::mongo::InvalidConfigException,
        "restart the process"
    );
}

USERVER_NAMESPACE_END

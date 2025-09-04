#include <storages/mongo/pool_impl.hpp>

#include <userver/dynamic_config/value.hpp>

#include <dynamic_config/variables/MONGO_CONGESTION_CONTROL_DATABASES_SETTINGS.hpp>
#include <dynamic_config/variables/MONGO_CONGESTION_CONTROL_ENABLED.hpp>
#include <dynamic_config/variables/MONGO_CONGESTION_CONTROL_SETTINGS.hpp>
#include <dynamic_config/variables/MONGO_CONNECTION_POOL_SETTINGS.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

PoolImpl::PoolImpl(std::string&& id, const PoolConfig& static_config, dynamic_config::Source config_source)
    : id_(std::move(id)),
      stats_verbosity_(static_config.stats_verbosity),
      config_source_(config_source),
      cc_sensor_(*this),
      cc_limiter_(*this),
      cc_controller_(
          id_,
          cc_sensor_,
          cc_limiter_,
          statistics_.congestion_control,
          static_config.cc_config,
          config_source,
          [](const dynamic_config::Snapshot& snapshot) {
              const auto& cfg = snapshot[::dynamic_config::MONGO_CONGESTION_CONTROL_SETTINGS];
              return congestion_control::v2::ConvertConfig(cfg);
          }
      ) {}

void PoolImpl::Start() {
    config_subscriber_ = config_source_.UpdateAndListen(this, "mongo_pool", &PoolImpl::OnConfigUpdate);
    cc_controller_.Start();
}

void PoolImpl::Stop() noexcept {
    cc_controller_.Stop();
    config_subscriber_.Unsubscribe();
}

void PoolImpl::OnConfigUpdate(const dynamic_config::Snapshot& config) {
    const bool cc_enabled =
        config[::dynamic_config::MONGO_CONGESTION_CONTROL_DATABASES_SETTINGS].GetOptional(id_).value_or(
            config[::dynamic_config::MONGO_CONGESTION_CONTROL_ENABLED]
        );
    cc_controller_.SetEnabled(cc_enabled);

    const auto new_pool_settings = config[::dynamic_config::MONGO_CONNECTION_POOL_SETTINGS].GetOptional(id_);
    if (new_pool_settings.has_value()) {
        PoolSettings ps;
        ps.max_size = new_pool_settings->max_size;
        ps.idle_limit = new_pool_settings->idle_limit;
        ps.initial_size = new_pool_settings->initial_size;
        ps.connecting_limit = new_pool_settings->connecting_limit;
        ps.Validate(id_);
        SetPoolSettings(ps);
    }
}

const std::string& PoolImpl::Id() const { return id_; }

const stats::PoolStatistics& PoolImpl::GetStatistics() const { return statistics_; }

stats::PoolStatistics& PoolImpl::GetStatistics() { return statistics_; }

dynamic_config::Snapshot PoolImpl::GetConfig() const { return config_source_.GetSnapshot(); }

StatsVerbosity PoolImpl::GetStatsVerbosity() const { return stats_verbosity_; }

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END

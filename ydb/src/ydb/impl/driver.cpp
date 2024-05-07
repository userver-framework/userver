#include "driver.hpp"

#include <ydb-cpp-sdk/client/driver/driver.h>
#include <ydb-cpp-sdk/client/extensions/solomon_stats/pull_connector.h>
#include <ydb-cpp-sdk/client/iam/iam.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/text_light.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/native_metrics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

Driver::Driver(std::string dbname, impl::DriverSettings settings)
    : dbname_(std::move(dbname)),
      dbpath_(settings.database),
      native_metrics_(std::make_unique<NMonitoring::TMetricRegistry>(
          NMonitoring::TLabels{})),
      retry_budget_(utils::RetryBudgetSettings{}) {
  NYdb::TDriverConfig driver_config;
  driver_config.SetEndpoint(settings.endpoint.data())
      .SetDatabase(settings.database.data())
      .SetBalancingPolicy(settings.prefer_local_dc
                              ? NYdb::EBalancingPolicy::UsePreferableLocation
                              : NYdb::EBalancingPolicy::UseAllNodes);

  if (settings.credentials_provider_factory) {
    driver_config.SetCredentialsProviderFactory(
        settings.credentials_provider_factory);
  } else if (settings.oauth_token.has_value()) {
    driver_config.SetAuthToken(settings.oauth_token->data());
  } else if (settings.iam_jwt_params.has_value()) {
    driver_config.UseSecureConnection().SetCredentialsProviderFactory(
        NYdb::CreateIamJwtParamsCredentialsProviderFactory(
            {.JwtContent = settings.iam_jwt_params->data()}));
  }

  driver_ = std::make_unique<NYdb::TDriver>(driver_config);
  NSolomonStatExtension::AddMetricRegistry(*driver_, native_metrics_.get());
}

Driver::~Driver() { driver_->Stop(true); }

const NYdb::TDriver& Driver::GetNativeDriver() const { return *driver_; }

const std::string& Driver::GetDbName() const { return dbname_; }

const std::string& Driver::GetDbPath() const { return dbpath_; }

utils::RetryBudget& Driver::GetRetryBudget() { return retry_budget_; }

void DumpMetric(utils::statistics::Writer& writer, const Driver& driver) {
  writer["native"] = *driver.native_metrics_;
  writer["retry_budget"] = driver.retry_budget_;
}

std::string JoinPath(std::string_view database_path, std::string_view path) {
  UASSERT(!utils::text::EndsWith(database_path, "/"));
  return utils::StrCat(database_path,
                       (utils::text::StartsWith(path, "/") ? "" : "/"), path);
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END

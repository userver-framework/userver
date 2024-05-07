#include <userver/ydb/settings.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {
namespace {

constexpr std::uint32_t kDefaultOperationRetries{3};
constexpr std::chrono::milliseconds kDefaultOperationTimeout{1'000};
constexpr std::chrono::milliseconds kDefaultCancellationTimeout{1'000};
constexpr std::chrono::milliseconds kDefaultClientTimeout{1'100};
constexpr std::chrono::milliseconds kDefaultGetSessionTimeout{5'000};

}  // namespace
}  // namespace ydb

namespace formats::parse {

ydb::OperationSettings Parse(const yaml_config::YamlConfig& config,
                             To<ydb::OperationSettings>) {
  ydb::OperationSettings result;
  result.retries =
      config["retries"].As<uint32_t>(ydb::kDefaultOperationRetries);
  result.operation_timeout_ms =
      config["operation-timeout"].As<std::chrono::milliseconds>(
          ydb::kDefaultOperationTimeout);
  result.cancel_after_ms = config["cancel-after"].As<std::chrono::milliseconds>(
      ydb::kDefaultCancellationTimeout);
  result.client_timeout_ms =
      config["client-timeout"].As<std::chrono::milliseconds>(
          ydb::kDefaultClientTimeout);
  result.get_session_timeout_ms =
      config["get-session-timeout"].As<std::chrono::milliseconds>(
          ydb::kDefaultGetSessionTimeout);
  result.tx_mode = ydb::TransactionMode::kSerializableRW;
  return result;
}

}  // namespace formats::parse

USERVER_NAMESPACE_END

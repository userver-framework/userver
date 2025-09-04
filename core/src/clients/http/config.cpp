#include <userver/clients/http/config.hpp>

#include <string_view>

#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <dynamic_config/variables/HTTP_CLIENT_CONNECT_THROTTLE.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
namespace {

void ParseTokenBucketSettings(
    size_t& limit,
    std::chrono::microseconds& rate,
    std::optional<int> limit_value,
    std::optional<int> per_second_value
) {
    limit = limit_value.value_or(impl::ThrottleConfig::kNoLimit);
    if (limit == 0) limit = impl::ThrottleConfig::kNoLimit;

    if (limit != impl::ThrottleConfig::kNoLimit) {
        const auto per_second = per_second_value.value_or(0);
        if (per_second) {
            rate = std::chrono::seconds{1};
            rate /= per_second;
        } else {
            limit = impl::ThrottleConfig::kNoLimit;
        }
    }
}

DeadlinePropagationConfig ParseDeadlinePropagationConfig(const yaml_config::YamlConfig& value) {
    DeadlinePropagationConfig result;
    result.update_header = value["set-deadline-propagation-header"].As<bool>(result.update_header);
    return result;
}

}  // namespace

CancellationPolicy Parse(yaml_config::YamlConfig value, formats::parse::To<CancellationPolicy>) {
    auto str = value.As<std::string>();
    if (str == "cancel") return CancellationPolicy::kCancel;
    if (str == "ignore") return CancellationPolicy::kIgnore;
    throw std::runtime_error("Invalid CancellationPolicy value: " + str);
}

ClientSettings Parse(const yaml_config::YamlConfig& value, formats::parse::To<ClientSettings>) {
    ClientSettings result;
    result.thread_name_prefix = value["thread-name-prefix"].As<std::string>(result.thread_name_prefix);
    result.io_threads = value["threads"].As<size_t>(result.io_threads);
    result.deadline_propagation = ParseDeadlinePropagationConfig(value);
    return result;
}

}  // namespace clients::http

namespace clients::http::impl {

ThrottleConfig Parse(const ::dynamic_config::http_client_connect_throttle::VariableType& value) {
    ThrottleConfig result;
    ParseTokenBucketSettings(
        result.http_connect_limit, result.http_connect_rate, value.http_limit, value.http_per_second
    );
    ParseTokenBucketSettings(
        result.https_connect_limit, result.https_connect_rate, value.https_limit, value.https_per_second
    );
    ParseTokenBucketSettings(
        result.per_host_connect_limit, result.per_host_connect_rate, value.per_host_limit, value.per_host_per_second
    );
    return result;
}

}  // namespace clients::http::impl

USERVER_NAMESPACE_END

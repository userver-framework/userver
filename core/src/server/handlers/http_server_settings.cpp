#include <server/handlers/http_server_settings.hpp>

#include <dynamic_config/variables/USERVER_LOG_REQUEST.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

CcCustomStatus Parse(const formats::json::Value& value, formats::parse::To<CcCustomStatus>) {
    return CcCustomStatus{
        static_cast<http::HttpStatus>(value["initial-status-code"].As<int>(429)),
        std::chrono::milliseconds{value["max-time-ms"].As<std::chrono::milliseconds::rep>(10000)},
    };
}

const dynamic_config::Key<CcCustomStatus> kCcCustomStatus{
    "USERVER_RPS_CCONTROL_CUSTOM_STATUS",
    dynamic_config::DefaultAsJsonString{"{}"},
};

}  // namespace server::handlers

USERVER_NAMESPACE_END

#include "driver_session.hpp"

#include <stdexcept>

#include <json/value.h>
#include <json/writer.h>

#include <yandex/taxi/userver/logging/log.hpp>
#include <yandex/taxi/userver/server/http/http_status.hpp>
#include <yandex/taxi/userver/storages/redis/component.hpp>
#include <yandex/taxi/userver/storages/redis/reply.hpp>

namespace driver_authorizer {
namespace handlers {

namespace {

const std::string kRedisClientName = "taximeter-base";

void ThrowUnmetRequirement(const std::string& what) {
  throw std::runtime_error("DriverSession handler requires " + what);
}

}  // namespace

DriverSession::DriverSession(const components::ComponentConfig& config,
                             const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      is_session_ttl_update_enabled_(
          config.ParseBool("session_ttl_update_enabled", true)),
      taxi_config_component_(context.FindComponent<components::TaxiConfig>()) {
  if (!taxi_config_component_) ThrowUnmetRequirement("taxi config");
  auto* redis_component = context.FindComponent<components::Redis>();
  if (!redis_component) ThrowUnmetRequirement("redis");
  redis_ptr_ = redis_component->Client(kRedisClientName);
  if (!redis_ptr_) ThrowUnmetRequirement(kRedisClientName + " redis client");
  LOG_INFO() << "Session TTL update "
             << (is_session_ttl_update_enabled_ ? "enabled" : "DISABLED");
}

const std::string& DriverSession::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

std::string DriverSession::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  auto db = request.GetArg("db");
  auto session = request.GetArg("session");

  if (db.empty() || session.empty()) {
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
    return "invalid or missing parameters";
  }

  const auto session_key = "DriverSession:" + db + ':' + session;
  auto session_request = redis_ptr_->Get(session_key);

  if (is_session_ttl_update_enabled_) {
    const std::chrono::seconds session_ttl{
        taxi_config_component_->Get()
            ->Get<driver_authorizer::TaxiConfig>()
            .driver_session_expire_seconds};
    LOG_TRACE() << "Updating session key '" << session_key << "' TTL to "
                << session_ttl.count() << " seconds";
    if (session_ttl > decltype(session_ttl)::zero()) {
      redis_ptr_->Expire(
          session_key, session_ttl, [](const storages::redis::ReplyPtr& reply) {
            if (!reply->IsOk()) {
              LOG_WARNING() << "EXPIRE failed with status " << reply->status
                            << " (" << reply->StatusString() << ')';
            } else if (!reply->data.IsInt()) {
              LOG_WARNING()
                  << "Unexpected EXPIRE reply: " << reply->data.ToString();
            } else if (reply->data.GetInt() != 1) {
              LOG_DEBUG() << "EXPIRE: key does not exist";
            }
          });
    } else {
      redis_ptr_->Persist(
          session_key, [](const storages::redis::ReplyPtr& reply) {
            if (!reply->IsOk()) {
              LOG_WARNING() << "PERSIST failed with status " << reply->status
                            << " (" << reply->StatusString() << ')';
            } else if (!reply->data.IsInt()) {
              LOG_WARNING()
                  << "Unexpected PERSIST reply: " << reply->data.ToString();
            } else if (reply->data.GetInt() == 1) {
              LOG_DEBUG() << "Session became persistent";
            }
          });
    }
  }

  auto session_reply = session_request.Get();
  if (!session_reply->IsOk()) {
    LOG_ERROR() << "Driver session request failed with status "
                << session_reply->status << " ("
                << session_reply->StatusString() << ')';
    request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
    return {};
  }

  std::string driver_id;
  if (session_reply->data.IsString()) {
    driver_id = session_reply->data.GetString();
    if (driver_id.size() >= 2 && driver_id.front() == '"' &&
        driver_id.back() == '"') {
      driver_id = driver_id.substr(1, driver_id.size() - 2);
    }
  } else if (!session_reply->data.IsNil()) {
    LOG_ERROR() << "Unexpected driver session with type '"
                << session_reply->data.GetTypeString()
                << "': " << session_reply->data.ToString();
    request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
    return {};
  }

  if (driver_id.empty()) {
    request.SetResponseStatus(server::http::HttpStatus::kUnauthorized);
    return "authorization failed";
  }

  Json::Value response_json(Json::objectValue);
  response_json["uuid"] = std::move(driver_id);

  return Json::FastWriter().write(response_json);
}

}  // namespace handlers
}  // namespace driver_authorizer

#include <userver/server/handlers/jemalloc.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/yaml_config/schema.hpp>
#include <utils/jemalloc.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {
std::string HandleRc(std::error_code ec) {
  if (ec)
    return "mallctl() returned error: " + ec.message() + "\n";
  else
    return "OK\n";
}
}  // namespace

Jemalloc::Jemalloc(const components::ComponentConfig& config,
                   const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true) {}

std::string Jemalloc::HandleRequestThrow(const http::HttpRequest& request,
                                         request::RequestContext&) const {
  const auto command = request.GetPathArg("command");
  if (command == "enable") {
    return HandleRc(utils::jemalloc::ProfActivate());
  } else if (command == "disable") {
    return HandleRc(utils::jemalloc::ProfDeactivate());
  } else if (command == "stat") {
    return utils::jemalloc::Stats();
  } else if (command == "dump") {
    return HandleRc(utils::jemalloc::ProfDump());
  } else if (command == "bg_threads_set_max") {
    size_t num_threads = 0;
    if (!request.HasArg("count")) {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      return "missing 'count' argument";
    }
    try {
      num_threads = utils::FromString<size_t>(request.GetArg("count"));
    } catch (const std::exception& ex) {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      return std::string{"invalid 'count' value: "} + ex.what();
    }
    return HandleRc(utils::jemalloc::SetMaxBgThreads(num_threads));
  } else if (command == "bg_threads_enable") {
    return HandleRc(utils::jemalloc::EnableBgThreads());
  } else if (command == "bg_threads_disable") {
    return HandleRc(utils::jemalloc::StopBgThreads());
  } else {
    return "Unsupported command";
  }
}

yaml_config::Schema Jemalloc::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-jemalloc config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END

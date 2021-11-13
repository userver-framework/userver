#include <userver/server/handlers/jemalloc.hpp>

#include <userver/logging/log.hpp>
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
  if (command == "enable")
    return HandleRc(utils::jemalloc::cmd::ProfActivate());
  else if (command == "disable")
    return HandleRc(utils::jemalloc::cmd::ProfDeactivate());
  else if (command == "stat")
    return utils::jemalloc::cmd::Stats();
  else if (command == "dump")
    return HandleRc(utils::jemalloc::cmd::ProfDump());
  else
    return "Unsupported command";
}

}  // namespace server::handlers

USERVER_NAMESPACE_END

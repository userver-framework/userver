#include <server/handlers/jemalloc.hpp>

#include <logging/log.hpp>
#include <utils/jemalloc.hpp>
#include <utils/strerror.hpp>

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

const std::string& Jemalloc::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

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

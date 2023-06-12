#include <userver/clients/http/plugin.hpp>

#include <clients/http/request_state.hpp>
#include <userver/clients/http/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

PluginRequest::PluginRequest(RequestState& state) : state_(state) {}

void PluginRequest::SetHeader(std::string_view name, std::string value) {
  state_.easy().add_header(name, value);
}

void PluginRequest::SetTimeout(std::chrono::milliseconds ms) {
  state_.SetEasyTimeout(ms);
}

Plugin::Plugin(std::string name) : name_(std::move(name)) {}

const std::string& Plugin::GetName() const { return name_; }

namespace impl {

PluginPipeline::PluginPipeline(
    const std::vector<utils::NotNull<Plugin*>>& plugins)
    : plugins_(plugins) {}

void PluginPipeline::HookCreateSpan(RequestState& request_state) {
  PluginRequest req(request_state);

  for (const auto& plugin : plugins_) {
    plugin->HookCreateSpan(req);
  }
}

void PluginPipeline::HookOnCompleted(RequestState& request_state,
                                     Response& response) {
  PluginRequest req(request_state);

  // NOLINTNEXTLINE(modernize-loop-convert)
  for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
    const auto& plugin = *it;
    plugin->HookOnCompleted(req, response);
  }
}

void PluginPipeline::HookPerformRequest(RequestState& request_state) {
  PluginRequest req(request_state);

  for (const auto& plugin : plugins_) {
    plugin->HookPerformRequest(req);
  }
}

}  // namespace impl

}  // namespace clients::http

USERVER_NAMESPACE_END

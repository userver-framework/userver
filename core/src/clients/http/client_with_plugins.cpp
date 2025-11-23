#include <userver/clients/http/client_with_plugins.hpp>

#include <userver/clients/http/client_core.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

ClientWithPlugins::ClientWithPlugins(
    utils::impl::InternalTag,
    std::shared_ptr<ClientCore> client_core,
    std::vector<utils::NotNull<clients::http::Plugin*>> plugins
)
    : client_core_(std::move(client_core)),
      plugins_(std::move(plugins))
{}

ClientWithPlugins::~ClientWithPlugins() { wts_.WaitForAllTokens(); }

Request ClientWithPlugins::CreateRequest() {
    auto request = client_core_->CreateRequest();
    request.SetPluginsList(plugins_);
    request.SetWaitToken(utils::impl::InternalTag{}, wts_.GetToken());
    return request;
}

}  // namespace clients::http

USERVER_NAMESPACE_END

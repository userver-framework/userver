#include <userver/clients/http/client_with_middlewares.hpp>

#include <userver/clients/http/client_core.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

ClientWithMiddlewares::ClientWithMiddlewares(
    utils::impl::InternalTag,
    std::shared_ptr<ClientCore> client_core,
    std::vector<utils::NotNull<clients::http::MiddlewareBase*>> middlewares
)
    : client_core_(std::move(client_core)),
      middlewares_(std::move(middlewares))
{}

ClientWithMiddlewares::~ClientWithMiddlewares() { wts_.WaitForAllTokens(); }

Request ClientWithMiddlewares::CreateRequest() {
    auto request = client_core_->CreateRequest();
    request.SetMiddlewaresList(middlewares_);
    request.SetWaitToken(utils::impl::InternalTag{}, wts_.GetToken());
    return request;
}

std::size_t ClientWithMiddlewares::GetActiveRequestCountDebug() const {
    return client_core_->GetActiveRequestCountDebug();
}

}  // namespace clients::http

USERVER_NAMESPACE_END

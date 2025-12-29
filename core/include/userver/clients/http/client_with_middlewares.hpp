#pragma once

/// @file userver/clients/http/client_with_middlewares.hpp
/// @brief @copybrief clients::http::ClientWithMiddlewares

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use clients::Http from clients/http.hpp instead
#endif

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/middlewares/base.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class ClientCore;

/// @ingroup userver_clients
///
/// @brief HTTP client that returns a HTTP request builder from
/// CreateRequest() with applied middlewares.
///
/// Usually retrieved from @ref components::HttpClient component.
/// Can also be created manually using @ref clients::http::Client::CreateHttpClient()
class ClientWithMiddlewares final : public Client {
public:
    /// @cond
    // For internal use only
    ClientWithMiddlewares(
        utils::impl::InternalTag,
        std::shared_ptr<ClientCore> client_core,
        std::vector<utils::NotNull<clients::http::MiddlewareBase*>> middlewares
    );
    /// @endcond

    ~ClientWithMiddlewares() override;

    /// @brief Returns a HTTP request builder type with preset values of
    /// User-Agent, middlewares and some of the Testsuite stuff (if any).
    ///
    /// @note This method is thread-safe despite being non-const.
    Request CreateRequest() override;

    /// @cond
    // For internal use only.
    std::size_t GetActiveRequestCountDebug() const;
    /// @endcond

private:
    std::shared_ptr<ClientCore> client_core_;
    std::vector<utils::NotNull<clients::http::MiddlewareBase*>> middlewares_;
    utils::impl::WaitTokenStorage wts_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END

#pragma once

/// @file userver/clients/http/client_with_plugins.hpp
/// @brief @copybrief clients::http::ClientWithPlugins

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use clients::Http from clients/http.hpp instead
#endif

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/plugin.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class ClientCore;

/// @ingroup userver_clients
///
/// @brief HTTP client that returns a HTTP request builder from
/// CreateRequest() with applied plugins.
///
/// Usually retrieved from @ref components::HttpClient component.
/// Can also be created manually using @ref clients::http::Client::CreateHttpClient()
class ClientWithPlugins final : public Client {
public:
    /// @cond
    // For internal use only
    ClientWithPlugins(
        utils::impl::InternalTag,
        std::shared_ptr<ClientCore> client_core,
        std::vector<utils::NotNull<clients::http::Plugin*>> plugins
    );
    /// @endcond

    ~ClientWithPlugins() override;

    /// @brief Returns a HTTP request builder type with preset values of
    /// User-Agent, plugins and some of the Testsuite stuff (if any).
    ///
    /// @note This method is thread-safe despite being non-const.
    Request CreateRequest() override;

private:
    std::shared_ptr<ClientCore> client_core_;
    std::vector<utils::NotNull<clients::http::Plugin*>> plugins_;
    utils::impl::WaitTokenStorage wts_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END

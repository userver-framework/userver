#pragma once

/// @file userver/chaotic/openapi/server/handler_base.hpp
/// @brief Base class template for chaotic-generated HTTP handlers

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <userver/chaotic/openapi/parameters_read.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/container.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::server {

namespace impl {

template <typename V>
concept ViewHasGetRequestBodyForLoggingJson = requires(const USERVER_NAMESPACE::formats::json::Value& body) {
    {
        V::GetRequestBodyForLogging(body)
    } -> std::convertible_to<std::string>;
};

template <typename V>
concept ViewHasGetRequestBodyForLoggingString = requires(const std::string& body) {
    {
        V::GetRequestBodyForLogging(body)
    } -> std::convertible_to<std::string>;
};

template <typename V>
concept ViewHasGetInvalidRequestBodyForLogging = requires(const USERVER_NAMESPACE::server::http::HttpRequest& req) {
    {
        V::GetInvalidRequestBodyForLogging(req)
    } -> std::convertible_to<std::string>;
};

template <typename V, typename R>
concept ViewHasGetResponseForLogging =
    requires(const R& resp, const std::string& s, USERVER_NAMESPACE::server::request::RequestContext& ctx) {
        {
            V::GetResponseForLogging(resp, s, ctx)
        } -> std::convertible_to<std::string>;
    };

}  // namespace impl

/// @brief Base class for generated HTTP handlers.
///
/// @tparam kName      Handler name, must be a `constexpr std::string_view`
///                    with static storage duration.
/// @tparam ErrorResponseBase  Base class of all throwable error responses for this
///                    operation, or `void` if there are none.
/// @tparam Request    Parsed request type for this operation.
/// @tparam Response   Response variant type for this operation.
/// @tparam HandlerTag Unique tag type for dependency injection keying.
///
/// The generated `Handler` subclass implements `Handle()`.
template <
    const std::string_view& Name,
    typename ErrorResponseBase,
    typename Request,
    typename Response,
    typename HandlerTag,
    typename View,
    typename HiddenArguments = void>
class BaseHandler final : public USERVER_NAMESPACE::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = Name;

    using Deps = chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;

    /// @brief Compute the URL as it would appear in logs, masking hidden args.
    ///
    /// Exposed as a static method so tests can call it without instantiating
    /// the handler (which requires a full component context).
    static std::string MaskUrlForLogging(const USERVER_NAMESPACE::server::http::HttpRequest& request) {
        if constexpr (std::is_void_v<HiddenArguments>) {
            return request.GetUrl();
        } else {
            return request.GetMaskedUrl(HiddenArguments::Contains);
        }
    }

    /// @brief Returns a View-specific request body log string, or std::nullopt
    /// to signal that the default HttpHandlerBase behaviour should be used.
    ///
    /// For JSON-body Views, `FromString(request_body)` is called without a
    /// try/except: on invalid JSON the exception propagates to the caller.
    /// The virtual override below catches it and delegates to
    /// `View::GetInvalidRequestBodyForLogging` when present.
    ///
    /// Exposed as a static method so tests can call it without instantiating
    /// the handler (which requires a full component context).
    static std::optional<std::string> TryGetBodyForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& /*request*/,
        const std::string& request_body
    ) {
        if constexpr (impl::ViewHasGetRequestBodyForLoggingJson<View>) {
            return View::GetRequestBodyForLogging(USERVER_NAMESPACE::formats::json::FromString(request_body));
        } else if constexpr (impl::ViewHasGetRequestBodyForLoggingString<View>) {
            return View::GetRequestBodyForLogging(request_body);
        }
        return std::nullopt;
    }

    BaseHandler(
        const USERVER_NAMESPACE::components::ComponentConfig& config,
        const USERVER_NAMESPACE::components::ComponentContext& context
    )
        : USERVER_NAMESPACE::server::handlers::HttpHandlerBase(config, context),
          factories_(context.FindComponent<FactoriesContainer>())
    {}

    ~BaseHandler() override = default;

private:
    // Guard against the old generated signature (const Request&) or other unexpected types.
    static_assert(
        !requires(const Request& r) { View::GetRequestBodyForLogging(r); },
        "View::GetRequestBodyForLogging(const Request&) is no longer supported. "
        "Use const formats::json::Value& (JSON body) or const std::string& (other body)."
    );
    static_assert(
        !(impl::ViewHasGetRequestBodyForLoggingJson<View> && impl::ViewHasGetRequestBodyForLoggingString<View>),
        "View must define at most one overload of GetRequestBodyForLogging."
    );
    static_assert(
        !impl::ViewHasGetInvalidRequestBodyForLogging<View> || impl::ViewHasGetRequestBodyForLoggingJson<View>,
        "View::GetInvalidRequestBodyForLogging requires "
        "View::GetRequestBodyForLogging(const formats::json::Value&)."
    );

    using Factories = chaotic::openapi::server::dependencies::Factories;
    using FactoriesContainer = USERVER_NAMESPACE::components::Container<Factories>;
    using HttpRequest = USERVER_NAMESPACE::server::http::HttpRequest;

    static Request ParseRequestOrThrow(const HttpRequest& http_request) {
        try {
            return ParseRequest(http_request, USERVER_NAMESPACE::chaotic::openapi::To<Request>{});
        } catch (const USERVER_NAMESPACE::server::handlers::ClientError&) {
            throw;
        } catch (const std::exception& e) {
            throw USERVER_NAMESPACE::server::handlers::ClientError(USERVER_NAMESPACE::server::handlers::ExternalBody{
                e.what()
            });
        }
    }

    std::string DoHandle(
        Request& request,
        HttpRequest& http_request,
        USERVER_NAMESPACE::server::request::RequestContext& context
    ) const {
        auto deps = factories_.Get().template Make<HandlerTag>();
        auto response = View::Handle(std::move(request), std::move(deps));
        auto serialized = SerializeResponse(response, http_request);
        if constexpr (impl::ViewHasGetResponseForLogging<View, Response>) {
            context.SetData<
                std::string>(std::string{kResponseLogKey}, View::GetResponseForLogging(response, serialized, context));
        }
        return serialized;
    }

    template <typename T>
    std::string HandleParsed(
        Request& request,
        HttpRequest& http_request,
        USERVER_NAMESPACE::server::request::RequestContext& context
    ) const {
        if constexpr (std::is_void_v<T>) {
            return DoHandle(request, http_request, context);
        } else {
            try {
                return DoHandle(request, http_request, context);
            } catch (ErrorResponseBase& e) {
                return SerializeResponse(std::move(e), http_request);
            }
        }
    }

    std::string HandleRequest(HttpRequest& http_request, USERVER_NAMESPACE::server::request::RequestContext& context)
        const final {
        auto request = ParseRequestOrThrow(http_request);
        return HandleParsed<ErrorResponseBase>(request, http_request, context);
    }

    std::string GetUrlForLogging(
        const HttpRequest& request,
        USERVER_NAMESPACE::server::request::RequestContext& context
    ) const override {
        if constexpr (std::is_void_v<HiddenArguments>) {
            return USERVER_NAMESPACE::server::handlers::HttpHandlerBase::GetUrlForLogging(request, context);
        } else {
            return MaskUrlForLogging(request);
        }
    }

    std::string GetRequestBodyForLogging(
        const HttpRequest& request,
        USERVER_NAMESPACE::server::request::RequestContext& context,
        const std::string& request_body
    ) const override {
        try {
            if (auto result = TryGetBodyForLogging(request, request_body)) {
                return std::move(*result);
            }
        } catch (...) {
            if constexpr (impl::ViewHasGetInvalidRequestBodyForLogging<View>) {
                return View::GetInvalidRequestBodyForLogging(request);
            }
        }
        return USERVER_NAMESPACE::server::handlers::HttpHandlerBase::GetRequestBodyForLogging(
            request,
            context,
            request_body
        );
    }

    std::string GetResponseDataForLogging(
        const HttpRequest& request,
        USERVER_NAMESPACE::server::request::RequestContext& context,
        const std::string& response_data
    ) const override {
        if constexpr (impl::ViewHasGetResponseForLogging<View, Response>) {
            if (auto* s = context.GetDataOptional<std::string>(kResponseLogKey)) {
                return *s;
            }
        }
        return USERVER_NAMESPACE::server::handlers::HttpHandlerBase::GetResponseDataForLogging(
            request,
            context,
            response_data
        );
    }

    static constexpr std::string_view kResponseLogKey = "chaotic_openapi_response_log";

    FactoriesContainer& factories_;
};

}  // namespace chaotic::openapi::server

USERVER_NAMESPACE_END

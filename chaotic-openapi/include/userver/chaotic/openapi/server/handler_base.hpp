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
#include <userver/server/handlers/formatted_error_data.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::server {

namespace impl {

inline const utils::AnyStorageDataTag<USERVER_NAMESPACE::server::request::StorageContext, std::string> kResponseLog;

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

// A View member named like a hook, whatever its signature; overload sets are not detected
template <typename V>
concept ViewDeclaresGetRequestBodyForLogging = requires { &V::GetRequestBodyForLogging; };

template <typename V>
concept ViewDeclaresGetInvalidRequestBodyForLogging = requires { &V::GetInvalidRequestBodyForLogging; };

template <typename V>
concept ViewDeclaresGetResponseForLogging = requires { &V::GetResponseForLogging; };

template <typename P>
concept HandlerPolicy = requires(
    const USERVER_NAMESPACE::server::http::HttpRequest& request,
    USERVER_NAMESPACE::server::request::RequestContext& context,
    const std::string& data,
    const USERVER_NAMESPACE::server::handlers::CustomHandlerException& exc
) {
    {
        P::GetRequestBodyForLogging(request, context, data)
    } -> std::same_as<std::optional<std::string>>;
    {
        P::GetResponseDataForLogging(request, context, data)
    } -> std::same_as<std::optional<std::string>>;
    {
        P::GetFormattedExternalErrorBody(exc)
    } -> std::same_as<std::optional<USERVER_NAMESPACE::server::handlers::FormattedErrorData>>;
};

}  // namespace impl

/// @brief Project-wide behaviour of generated handlers, named once per handlers target.
///
/// A policy derives from DefaultHandlerPolicy and redefines the hooks it needs. Each hook
/// returns `std::nullopt` to fall back to the userver default. A View's own logging hooks
/// take precedence over the policy.
struct DefaultHandlerPolicy {
    /// Request body as written to the logs
    static std::optional<std::string> GetRequestBodyForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& /*request*/,
        USERVER_NAMESPACE::server::request::RequestContext& /*context*/,
        const std::string& /*request_body*/
    ) {
        return std::nullopt;
    }

    /// Response body as written to the logs
    static std::optional<std::string> GetResponseDataForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& /*request*/,
        USERVER_NAMESPACE::server::request::RequestContext& /*context*/,
        const std::string& /*response_data*/
    ) {
        return std::nullopt;
    }

    /// Body of an error response the operation does not declare
    static std::optional<USERVER_NAMESPACE::server::handlers::FormattedErrorData> GetFormattedExternalErrorBody(
        const USERVER_NAMESPACE::server::handlers::CustomHandlerException& /*exc*/
    ) {
        return std::nullopt;
    }
};

/// @brief Base class for generated HTTP handlers.
///
/// @tparam kName      Handler name, must be a `constexpr std::string_view`
///                    with static storage duration.
/// @tparam ErrorResponseBase  Base class of all throwable error responses for this
///                    operation, or `void` if there are none.
/// @tparam Request    Parsed request type for this operation.
/// @tparam Response   Response variant type for this operation.
/// @tparam HandlerTag Unique tag type for dependency injection keying.
/// @tparam Policy     Project-wide logging and error rendering, see DefaultHandlerPolicy.
///
/// The generated `Handler` subclass implements `Handle()`.
template <
    const std::string_view& Name,
    typename ErrorResponseBase,
    typename Request,
    typename Response,
    typename HandlerTag,
    typename View,
    typename HiddenArguments = void,
    typename Policy = DefaultHandlerPolicy>
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

    /// @brief Request body log string of the View or the Policy, or std::nullopt
    /// for the default HttpHandlerBase behaviour.
    static std::optional<std::string> RequestBodyForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& request,
        USERVER_NAMESPACE::server::request::RequestContext& context,
        const std::string& request_body
    ) {
        try {
            if (auto result = TryGetBodyForLogging(request, request_body)) {
                return result;
            }
        } catch (...) {
            if constexpr (impl::ViewHasGetInvalidRequestBodyForLogging<View>) {
                return View::GetInvalidRequestBodyForLogging(request);
            }
        }
        return Policy::GetRequestBodyForLogging(request, context, request_body);
    }

    /// @brief Response body log string of the View or the Policy, or std::nullopt
    /// for the default HttpHandlerBase behaviour.
    static std::optional<std::string> ResponseDataForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& request,
        USERVER_NAMESPACE::server::request::RequestContext& context,
        const std::string& response_data
    ) {
        if constexpr (impl::ViewHasGetResponseForLogging<View, Response>) {
            if (auto* s = context.GetDataOptional(impl::kResponseLog)) {
                return *s;
            }
        }
        return Policy::GetResponseDataForLogging(request, context, response_data);
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
    static_assert(
        !impl::ViewDeclaresGetRequestBodyForLogging<View> || impl::ViewHasGetRequestBodyForLoggingJson<View> ||
            impl::ViewHasGetRequestBodyForLoggingString<View>,
        "View::GetRequestBodyForLogging matches no supported signature: "
        "static std::string GetRequestBodyForLogging(const formats::json::Value& body) (JSON body) or "
        "static std::string GetRequestBodyForLogging(const std::string& body) (other body)."
    );
    static_assert(
        !impl::ViewDeclaresGetInvalidRequestBodyForLogging<View> || impl::ViewHasGetInvalidRequestBodyForLogging<View>,
        "View::GetInvalidRequestBodyForLogging matches no supported signature: "
        "static std::string GetInvalidRequestBodyForLogging(const server::http::HttpRequest& request)."
    );
    static_assert(
        !impl::ViewDeclaresGetResponseForLogging<View> || impl::ViewHasGetResponseForLogging<View, Response>,
        "View::GetResponseForLogging matches no supported signature: "
        "static std::string GetResponseForLogging(const Response& response, const std::string& serialized_response, "
        "server::request::RequestContext& context)."
    );
    static_assert(
        impl::HandlerPolicy<Policy>,
        "Policy must provide the hooks of chaotic::openapi::server::DefaultHandlerPolicy with the same signatures, "
        "derive it from DefaultHandlerPolicy."
    );
    static_assert(
        requires(Request&& r, Deps&& d, USERVER_NAMESPACE::server::request::RequestContext& ctx) {
            {
                View::Handle(std::move(r), std::move(d), ctx)
            } -> std::convertible_to<Response>;
        },
        "View::Handle must accept server::request::RequestContext as the third parameter: "
        "static Response Handle(Request&& request, Deps&& deps, RequestContext& context). "
        "The legacy 2-argument Handle(Request&&, Deps&&) is no longer supported."
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
        auto response = View::Handle(std::move(request), std::move(deps), context);
        auto serialized = SerializeResponse(response, http_request);
        if constexpr (impl::ViewHasGetResponseForLogging<View, Response>) {
            context.EmplaceData(impl::kResponseLog, View::GetResponseForLogging(response, serialized, context));
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
        if (auto result = RequestBodyForLogging(request, context, request_body)) {
            return std::move(*result);
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
        if (auto result = ResponseDataForLogging(request, context, response_data)) {
            return std::move(*result);
        }
        return USERVER_NAMESPACE::server::handlers::HttpHandlerBase::GetResponseDataForLogging(
            request,
            context,
            response_data
        );
    }

    USERVER_NAMESPACE::server::handlers::FormattedErrorData GetFormattedExternalErrorBody(
        const USERVER_NAMESPACE::server::handlers::CustomHandlerException& exc
    ) const override {
        if (auto result = Policy::GetFormattedExternalErrorBody(exc)) {
            return std::move(*result);
        }
        return USERVER_NAMESPACE::server::handlers::HttpHandlerBase::GetFormattedExternalErrorBody(exc);
    }

    FactoriesContainer& factories_;
};

}  // namespace chaotic::openapi::server

USERVER_NAMESPACE_END

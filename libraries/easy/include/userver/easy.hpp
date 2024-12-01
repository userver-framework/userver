#pragma once

/// @file userver/easy.hpp
/// @brief Headers of an library for `easy` prototyping

#include <functional>
#include <string>
#include <string_view>

#include <userver/clients/http/client.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_list.hpp>
#include <userver/formats/json.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

#include <userver/storages/postgres/cluster.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for `easy` library
namespace easy {

namespace impl {

class DependenciesBase : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "easy-dependencies";
    using components::ComponentBase::ComponentBase;
    virtual ~DependenciesBase();
};

}  // namespace impl

/// @brief easy::HttpWith like class with erased dependencies information that should be used only in dependency
/// registration functions; use easy::HttpWith if not making a new dependency class.
class HttpBase final {
public:
    using Callback = std::function<std::string(const server::http::HttpRequest&, const impl::DependenciesBase&)>;

    /// Sets the default Content-Type header for all the routes
    void DefaultContentType(http::ContentType content_type);

    /// Register an HTTP handler by `path` that supports the `methods` HTTP methods
    void Route(std::string_view path, Callback&& func, std::initializer_list<server::http::HttpMethod> methods);

    /// Append a component to the component list of a service
    template <class Component>
    bool TryAddComponent(std::string_view name, std::string_view config) {
        if (component_list_.Contains(name)) {
            return false;
        }

        component_list_.Append<Component>(name);
        AddComponentConfig(name, config);
        return true;
    }

    template <class Component>
    bool TryAddComponent(std::string_view name) {
        if (component_list_.Contains(name)) {
            return false;
        }

        component_list_.Append<Component>(name);
        return true;
    }

    /// Stores the schema for further retrieval from GetDbSchema()
    void DbSchema(std::string_view schema);

    /// @returns the \b last schema that was provided to the easy::HttpWith or easy::HttpBase
    static const std::string& GetDbSchema() noexcept;

    /// Set the HTTP server listen port, default is 8080.
    void Port(std::uint16_t port);

    /// Set the logging level for the service
    void LogLevel(logging::Level level);

private:
    template <class>
    friend class HttpWith;

    void AddComponentConfig(std::string_view name, std::string_view config);

    HttpBase(int argc, const char* const argv[]);
    ~HttpBase();

    class Handle;

    const int argc_;
    const char* const* argv_;
    std::string static_config_;
    components::ComponentList component_list_;

    std::uint16_t port_ = 8080;
    logging::Level level_ = logging::Level::kDebug;
};

/// Class that combines dependencies passed to HttpWith into a single type, that is passed to callbacks.
///
/// @see @ref scripts/docs/en/userver/libraries/easy.md
template <class... Dependency>
class Dependencies final : public impl::DependenciesBase, public Dependency... {
public:
    Dependencies(const components::ComponentConfig& config, const components::ComponentContext& context)
        : DependenciesBase{config, context}, Dependency{context}... {}

    static void RegisterOn(HttpBase& app) { (Dependency::RegisterOn(app), ...); }
};

/// @brief Class for describing the service functionality in simple declarative way that generates static configs,
/// applies schemas.
///
/// @see @ref scripts/docs/en/userver/libraries/easy.md
template <class Deps = Dependencies<>>
class HttpWith final {
public:
    using Dependency = std::conditional_t<std::is_base_of_v<impl::DependenciesBase, Deps>, Deps, Dependencies<Deps>>;

    /// Helper class that can store any callback of the following signatures:
    ///
    /// * formats::json::Value(formats::json::Value, const Dependency&)
    /// * formats::json::Value(formats::json::Value)
    /// * formats::json::Value(const HttpRequest&, const Dependency&)
    /// * std::string(const HttpRequest&, const Dependency&)
    /// * formats::json::Value(const HttpRequest&)
    /// * std::string(const HttpRequest&)
    ///
    /// If callback returns formats::json::Value then the deafult content type is set to `application/json`
    class Callback final {
    public:
        template <class Function>
        Callback(Function func);

        HttpBase::Callback Extract() && noexcept { return std::move(func_); }

    private:
        HttpBase::Callback func_;
    };

    HttpWith(int argc, const char* const argv[]) : impl_(argc, argv) {
        impl_.TryAddComponent<Dependency>(Dependency::kName);
    }
    ~HttpWith() { Dependency::RegisterOn(impl_); }

    /// @copydoc HttpBase::DefaultContentType
    HttpWith& DefaultContentType(http::ContentType content_type) {
        return (impl_.DefaultContentType(content_type), *this);
    }

    /// @copydoc HttpBase::Route
    HttpWith& Route(
        std::string_view path,
        Callback&& func,
        std::initializer_list<server::http::HttpMethod> methods =
            {
                server::http::HttpMethod::kGet,
                server::http::HttpMethod::kPost,
                server::http::HttpMethod::kDelete,
                server::http::HttpMethod::kPut,
                server::http::HttpMethod::kPatch,
            }
    ) {
        impl_.Route(path, std::move(func).Extract(), methods);
        return *this;
    }

    /// Register an HTTP handler by `path` that supports the HTTP GET method.
    HttpWith& Get(std::string_view path, Callback&& func) {
        impl_.Route(path, std::move(func).Extract(), {server::http::HttpMethod::kGet});
        return *this;
    }

    /// Register an HTTP handler by `path` that supports the HTTP POST method.
    HttpWith& Post(std::string_view path, Callback&& func) {
        impl_.Route(path, std::move(func).Extract(), {server::http::HttpMethod::kPost});
        return *this;
    }

    /// Register an HTTP handler by `path` that supports the HTTP DELETE method.
    HttpWith& Del(std::string_view path, Callback&& func) {
        impl_.Route(path, std::move(func).Extract(), {server::http::HttpMethod::kDelete});
        return *this;
    }

    /// Register an HTTP handler by `path` that supports the HTTP PUT method.
    HttpWith& Put(std::string_view path, Callback&& func) {
        impl_.Route(path, std::move(func).Extract(), {server::http::HttpMethod::kPut});
        return *this;
    }

    /// Register an HTTP handler by `path` that supports the HTTP PATCH method.
    HttpWith& Patch(std::string_view path, Callback&& func) {
        impl_.Route(path, std::move(func).Extract(), {server::http::HttpMethod::kPatch});
        return *this;
    }

    /// @copydoc HttpBase::Schema
    HttpWith& DbSchema(std::string_view schema) {
        impl_.DbSchema(schema);
        return *this;
    }

    /// @copydoc HttpBase::Port
    HttpWith& Port(std::uint16_t port) {
        impl_.Port(port);
        return *this;
    }

    /// @copydoc HttpBase::LogLevel
    HttpWith& LogLevel(logging::Level level) {
        impl_.LogLevel(level);
        return *this;
    }

private:
    HttpBase impl_;
};

template <class Deps>
template <class Function>
HttpWith<Deps>::Callback::Callback(Function func) {
    using server::http::HttpRequest;

    constexpr unsigned kMatches =
        (std::is_invocable_r_v<formats::json::Value, Function, formats::json::Value, const Dependency&> << 0) |
        (std::is_invocable_r_v<formats::json::Value, Function, formats::json::Value> << 1) |
        (std::is_invocable_r_v<formats::json::Value, Function, const HttpRequest&, const Dependency&> << 2) |
        (std::is_invocable_r_v<std::string, Function, const HttpRequest&, const Dependency&> << 3) |
        (std::is_invocable_r_v<formats::json::Value, Function, const HttpRequest&> << 4) |
        (std::is_invocable_r_v<std::string, Function, const HttpRequest&> << 5);
    static_assert(
        kMatches,
        "Failed to find a matching signature. See the easy::HttpWith::Callback docs for info on "
        "supported signatures"
    );
    constexpr bool has_single_match = ((kMatches & (kMatches - 1)) == 0);
    static_assert(
        has_single_match,
        "Found more than one matching signature, probably due to `auto` usage in parameters. See "
        "the easy::HttpWith::Callback docs for info on supported signatures"
    );

    if constexpr (kMatches & 1) {
        func_ = [f = std::move(func)](const HttpRequest& req, const impl::DependenciesBase& deps) {
            req.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
            return formats::json::ToString(
                f(formats::json::FromString(req.RequestBody()), static_cast<const Dependency&>(deps))
            );
        };
    } else if constexpr (kMatches & 2) {
        func_ = [f = std::move(func)](const HttpRequest& req, const impl::DependenciesBase&) {
            req.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
            return formats::json::ToString(f(formats::json::FromString(req.RequestBody())));
        };
    } else if constexpr (kMatches & 4) {
        func_ = [f = std::move(func)](const HttpRequest& req, const impl::DependenciesBase& deps) {
            req.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
            return formats::json::ToString(f(req, static_cast<const Dependency&>(deps)));
        };
    } else if constexpr (kMatches & 8) {
        func_ = [f = std::move(func)](const HttpRequest& req, const impl::DependenciesBase& deps) {
            return f(req, static_cast<const Dependency&>(deps));
        };
    } else if constexpr (kMatches & 16) {
        func_ = [f = std::move(func)](const HttpRequest& req, const impl::DependenciesBase&) {
            req.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
            return formats::json::ToString(f(req));
        };
    } else {
        static_assert(kMatches & 32);
        func_ = [f = std::move(func)](const HttpRequest& req, const impl::DependenciesBase&) { return f(req); };
    }
}

/// @brief Dependency class that provides a PostgreSQL cluster client.
class PgDep {
public:
    explicit PgDep(const components::ComponentContext& context);
    storages::postgres::Cluster& pg() const noexcept { return *pg_cluster_; }
    static void RegisterOn(HttpBase& app);

private:
    storages::postgres::ClusterPtr pg_cluster_;
};

/// @brief Dependency class that provides a Http client.
class HttpDep {
public:
    explicit HttpDep(const components::ComponentContext& context);
    clients::http::Client& http() { return http_; }
    static void RegisterOn(HttpBase& app);

private:
    clients::http::Client& http_;
};

}  // namespace easy

template <class... Components>
inline constexpr auto components::kConfigFileMode<easy::Dependencies<Components...>> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END

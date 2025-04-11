#pragma once

/// @file userver/server/handlers/jemalloc.hpp
/// @brief @copybrief server::handlers::Jemalloc

#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that controls the jemalloc allocator.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler jemalloc component config
///
/// ## Schema
/// Set an URL path argument `command` to one of the following values:
/// * `stat` - to get jemalloc stats
/// * `enable` - to start memory profiling
/// * `disable` - to stop memory profiling
/// * `dump` - to get jemalloc profiling dump
/// * `bg_threads_set_max` - to set maximum number of background threads
/// * `bg_threads_enable` - to start background threads
/// * `bg_threads_disable` - to *synchronously* stop background threads

// clang-format on

class Jemalloc final : public HttpHandlerBase {
public:
    enum class Command {
        kStat,
        kEnable,
        kDisable,
        kDump,
        kBgThreadsSetMax,
        kBgThreadsEnable,
        kBgThreadsDisable,
    };
    static std::optional<Command> GetCommandFromString(std::string_view str);
    static std::string ListCommands();

    Jemalloc(const components::ComponentConfig&, const components::ComponentContext&);

    /// @ingroup userver_component_names
    /// @brief The default name of server::handlers::Jemalloc
    static constexpr std::string_view kName = "handler-jemalloc";

    std::string HandleRequestThrow(const http::HttpRequest&, request::RequestContext&) const override;

    static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::Jemalloc> = true;

USERVER_NAMESPACE_END

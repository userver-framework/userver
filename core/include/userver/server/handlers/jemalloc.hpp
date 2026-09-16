#pragma once

/// @file userver/server/handlers/jemalloc.hpp
/// @brief @copybrief server::handlers::Jemalloc

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that controls the jemalloc allocator.
///
/// The component also implements the `pprof` remote profiling protocol, so that
/// `jeprof` can fetch and symbolize a heap profile without a copy of the
/// service binary. The service symbolizes its own addresses, which removes both
/// problems of the binary-based flow: hauling a multi-gigabyte binary off the
/// build cache, and getting silently wrong symbols when that binary does not
/// match the deployed one.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet core/src/components/common_server_component_list_test.cpp  Sample handler jemalloc component config
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
/// * `heap` (`GET`) - the jemalloc heap profile dump, in the `pprof` format
/// * `cmdline` (`GET`, Linux) - the process arguments separated by null bytes
/// * `symbol` (`GET`) - reports that symbolization is available
/// * `symbol` (`POST`) - maps the `+`-separated hex addresses of the request
///   body to function names, one `0x<address> <name>` line per resolved address
///
/// ## Usage of the `pprof` protocol
/// @code
/// jeprof --raw http://localhost:1188/service/jemalloc/pprof/heap > out.raw
/// jeprof --text out.raw
/// @endcode
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
        kHeap,
        kCmdline,
        kSymbol,
    };
    static std::optional<Command> GetCommandFromString(std::string_view str);
    static std::string ListCommands();

    Jemalloc(const components::ComponentConfig&, const components::ComponentContext&);

    /// @ingroup userver_component_names
    /// @brief The default name of server::handlers::Jemalloc
    static constexpr std::string_view kName = "handler-jemalloc";

    std::string HandleRequestThrow(const http::HttpRequest&, request::RequestContext&) const override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    engine::TaskProcessor& fs_task_processor_;
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::Jemalloc> = true;

USERVER_NAMESPACE_END

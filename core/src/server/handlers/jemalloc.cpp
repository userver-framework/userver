#include <userver/server/handlers/jemalloc.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/strerror.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/schema.hpp>
#include <utils/jemalloc.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

#ifdef USERVER_FEATURE_JEMALLOC_ENABLED
std::string HandleRc(const http::HttpRequest& request, std::error_code ec) {
    if (ec) {
        request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
        return "mallctl() returned error: " + ec.message() + "\n";
    }
    return "OK\n";
}
#endif

constexpr utils::TrivialBiMap kStrToCommand = [](auto selector) {
    using Command = Jemalloc::Command;
    return selector()
        .Case("stat", Command::kStat)
        .Case("enable", Command::kEnable)
        .Case("disable", Command::kDisable)
        .Case("dump", Command::kDump)
        .Case("bg_threads_set_max", Command::kBgThreadsSetMax)
        .Case("bg_threads_enable", Command::kBgThreadsEnable)
        .Case("bg_threads_disable", Command::kBgThreadsDisable);
};

}  // namespace

std::optional<Jemalloc::Command> Jemalloc::GetCommandFromString(std::string_view str) {
    return kStrToCommand.TryFind(str);
}

std::string Jemalloc::ListCommands() { return kStrToCommand.DescribeFirst(); }

Jemalloc::Jemalloc(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true) {}

std::string Jemalloc::HandleRequestThrow(const http::HttpRequest& request, request::RequestContext&) const {
#ifndef USERVER_FEATURE_JEMALLOC_ENABLED
    request.SetResponseStatus(server::http::HttpStatus::kNotImplemented);
    return "'jemalloc' profiling is not available. Is USERVER_FEATURE_JEMALLOC defined? Is the platform supported?\n";
#else
    const auto opt_command = GetCommandFromString(request.GetPathArg("command"));
    if (!opt_command) {
        request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        return fmt::format("Unsupported command. Supported commands are: {}\n", ListCommands());
    }
    switch (*opt_command) {
        case Command::kStat:
            return utils::jemalloc::Stats();
        case Command::kEnable:
            if (!utils::jemalloc::IsProfilingEnabledViaEnv()) {
                request.SetResponseStatus(server::http::HttpStatus::kServiceUnavailable);
                return "'jemalloc' profiling is not available because the service was not started with a 'MALLOC_CONF' "
                       "environment variable that contain 'prof:true'";
            }
            return HandleRc(request, utils::jemalloc::ProfActivate());
        case Command::kDisable:
            return HandleRc(request, utils::jemalloc::ProfDeactivate());
        case Command::kDump:
            return HandleRc(request, utils::jemalloc::ProfDump());
        case Command::kBgThreadsSetMax: {
            size_t num_threads = 0;
            if (!request.HasArg("count")) {
                request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
                return "missing 'count' argument";
            }
            try {
                num_threads = utils::FromString<size_t>(request.GetArg("count"));
            } catch (const std::exception& ex) {
                request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
                return std::string{"invalid 'count' value: "} + ex.what();
            }
            return HandleRc(request, utils::jemalloc::SetMaxBgThreads(num_threads));
        }
        case Command::kBgThreadsEnable:
            return HandleRc(request, utils::jemalloc::EnableBgThreads());
        case Command::kBgThreadsDisable:
            return HandleRc(request, utils::jemalloc::StopBgThreads());
    }

    UINVARIANT(false, "Unsupported command");
#endif
}

yaml_config::Schema Jemalloc::GetStaticConfigSchema() {
    auto schema = HttpHandlerBase::GetStaticConfigSchema();
    schema.UpdateDescription("handler-jemalloc config");
    return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END

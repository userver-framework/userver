#include <userver/server/handlers/jemalloc.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/engine/task/current_task.hpp>
#include <userver/fs/temp_file.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/impl/pprof.hpp>
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

std::string HandleHeap(const http::HttpRequest& request, engine::TaskProcessor& fs_task_processor) {
    if (!impl::IsJemallocProfilingEnabledViaEnv()) {
        return impl::JemallocProfilingUnavailable(request);
    }

    const auto dump_file = fs::TempFile::Create("/tmp", "jeprof", fs_task_processor);
    const auto ec = utils::jemalloc::ProfDumpTo(dump_file.GetPath());
    if (ec) {
        request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
        return "mallctl() returned error: " + ec.message() + "\n";
    }

    return impl::PprofReadDump(request, fs_task_processor, dump_file.GetPath());
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
        .Case("bg_threads_disable", Command::kBgThreadsDisable)
        .Case("heap", Command::kHeap)
        .Case("symbol", Command::kSymbol);
};

constexpr std::string_view kCommandArg = "command";

}  // namespace

std::optional<Jemalloc::Command> Jemalloc::GetCommandFromString(std::string_view str) {
    return kStrToCommand.TryFind(str);
}

std::string Jemalloc::ListCommands() { return kStrToCommand.DescribeFirst(); }

Jemalloc::Jemalloc(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      fs_task_processor_(engine::current_task::GetBlockingTaskProcessor())
{}

std::string Jemalloc::HandleRequestThrow(const http::HttpRequest& request, request::RequestContext&) const {
    const auto opt_command = GetCommandFromString(request.GetPathArg(kCommandArg));
    if (!opt_command) {
        request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        return fmt::format("Unsupported command. Supported commands are: {}\n", ListCommands());
    }

    auto pprof_response = impl::TryHandlePprofCommand(request, opt_command.value());
    if (pprof_response.has_value()) {
        return std::move(*pprof_response);
    }

#ifndef USERVER_FEATURE_JEMALLOC_ENABLED
    request.SetResponseStatus(server::http::HttpStatus::kNotImplemented);
    return "'jemalloc' profiling is not available. Is USERVER_FEATURE_JEMALLOC defined? Is the platform supported?\n";
#else
    switch (opt_command.value()) {
        case Command::kStat:
            return utils::jemalloc::Stats();
        case Command::kEnable:
            if (!impl::IsJemallocProfilingEnabledViaEnv()) {
                return impl::JemallocProfilingUnavailable(request);
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
        case Command::kHeap:
            return HandleHeap(request, fs_task_processor_);
        case Command::kSymbol:
            break;  // handled above
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

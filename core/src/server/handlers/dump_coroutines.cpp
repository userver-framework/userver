#include <userver/server/handlers/dump_coroutines.hpp>

#include <engine/task/task_processor.hpp>
#include <engine/tracer_plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

struct DumpCoroutines::Impl {
    const engine::TracePlugin* trace_plugin;
};

DumpCoroutines::DumpCoroutines(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context, true)
{
    impl_->trace_plugin = &engine::current_task::GetTaskProcessor().GetTracePlugin();
}

DumpCoroutines::~DumpCoroutines() = default;

formats::json::Value
DumpCoroutines::HandleRequestJsonThrow(const http::HttpRequest&, const formats::json::Value&, request::RequestContext&)
    const {
    auto tasks = impl_->trace_plugin->GetAllTasks();
    formats::json::ValueBuilder vb;
    for (const auto& task : tasks) {
        vb.PushBack(formats::json::MakeObject(
            "task",
            task.task,
            "span",
            task.span_name,
            "link",
            task.link,
            "stacktrace",
            to_string(task.last_bt_before_sleep)
        ));
    }
    return vb.ExtractValue();
}

}  // namespace server::handlers

USERVER_NAMESPACE_END

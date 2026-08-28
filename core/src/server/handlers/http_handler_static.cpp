#include <userver/server/handlers/http_handler_static.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/fs/blocking/c_file.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/small_string.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <dynamic_config/variables/USERVER_FILES_CONTENT_TYPE_MAP.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/handlers/http_handler_static.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

using Queue = concurrent::SpscQueue<std::string>;
using SearchPath = utils::SmallString<4096>;

constexpr std::size_t kReadBufferSize = 8192;

SearchPath BuildSearchPath(const http::HttpRequest& request) {
    SearchPath search_path;
    search_path.reserve(request.GetRequestPath().size());
    for (std::size_t i = 0; i < request.PathArgCount(); ++i) {
        search_path += '/';
        search_path += request.GetPathArg(i);
    }
    return search_path;
}

void DoSendChunks(http::ResponseBodyStream& stream, Queue::Consumer consumer) {
    std::string data;
    while (!engine::current_task::ShouldCancel() && consumer.Pop(data)) {
        stream.PushBodyChunk(std::move(data), {});
    }
}

void DoReadFile(std::filesystem::path path, Queue::Producer producer) {
    fs::blocking::CFile file{path.native(), fs::blocking::OpenFlag::kRead};
    std::string buf;

    while (!engine::current_task::ShouldCancel()) {
        buf.resize(kReadBufferSize);
        const auto read_bytes = file.Read(buf);
        if (!read_bytes) {
            if (!std::feof(file.GetNative())) {
                LOG_ERROR("Failed to read data for file '{}'", path.filename().native());
            }
            return;
        }
        if (read_bytes != buf.size()) {
            buf.resize(read_bytes);
        }
        if (!producer.Push(std::exchange(buf, {}))) {
            return;
        }
    }
}

}  // namespace

HttpHandlerStatic::ResolvedFile HttpHandlerStatic::ResolveFile(const http::HttpRequest& request) const {
    auto search_path = BuildSearchPath(request);
    LOG_DEBUG() << "search_path: " << std::string_view{search_path};

    ResolvedFile result;
    result.file = storage_.TryGetFile(search_path);
    if (!result.file && !directory_file_.empty()) {
        if (directory_file_.front() == '/') {
            result.file = storage_.TryGetFile(directory_file_);
        } else {
            if (search_path.empty() || search_path.back() != '/') {
                search_path += '/';
            }
            search_path += directory_file_;
            LOG_DEBUG() << "search_path 2: " << std::string_view{search_path};
            result.file = storage_.TryGetFile(search_path);
        }
    }
    if (!result.file) {
        result.file = storage_.TryGetFile(not_found_file_);
        result.is_not_found = true;
    }
    return result;
}

HttpHandlerStatic::HttpHandlerStatic(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : HttpHandlerBase(config, context),
      config_(context.FindComponent<components::DynamicConfig>().GetSource()),
      storage_(context
                   .FindComponent<components::FsCache>(config["fs-cache-component"].As<std::string>("fs-cache-component"
                   ))
                   .GetClient()),
      cache_age_(config["expires"].As<std::chrono::seconds>(600)),
      directory_file_(config["directory-file"].As<std::string>("index.html")),
      not_found_file_(config["not-found-file"].As<std::string>("/404.html")),
      fs_task_processor_(GetFsTaskProcessor(config, context))
{
    if (!HttpHandlerBase::IsBodyStreamingEnabledInConfig()) {
        LOG_INFO() << "'response-body-stream: false' is ignored for " << HttpHandlerBase::HandlerName();
    }
}

void HttpHandlerStatic::HandleMaybeStreamRequest(http::HttpRequest& http_request, request::RequestContext&) const {
    const auto resolved = ResolveFile(http_request);
    auto& response = http_request.GetHttpResponse();
    if (!resolved.file) {
        response.SetStatusNotFound();
        response.SetData("File not found");
        return;
    }

    if (resolved.is_not_found) {
        response.SetStatusNotFound();
    }

    {
        const auto config = config_.GetSnapshot();
        response.SetHeader(USERVER_NAMESPACE::http::headers::kExpires, std::to_string(cache_age_.count()));
        response.SetContentType(config[::dynamic_config::USERVER_FILES_CONTENT_TYPE_MAP][resolved.file->extension]);
    }

    std::visit(
        utils::Overloaded{
            [&](const std::string& body) {
                // Aliasing shared_ptr: keep FileInfoWithData alive, point at its string member — no body copy.
                response.SetSharedData(std::shared_ptr<const std::string>{resolved.file, &body});
            },
            [&](const std::filesystem::path& path) {
                http::ResponseBodyStream stream(response);
                stream.SetEndOfHeaders();

                constexpr std::size_t kMaxQueueSize = 32;  // up to ~0.25MB with kReadBufferSize
                auto queue = Queue::Create(kMaxQueueSize);
                auto send_task = engine::AsyncNoTracing(fs_task_processor_, DoReadFile, path, queue->GetProducer());

                DoSendChunks(stream, queue->GetConsumer());
            },
        },
        resolved.file->data_or_path
    );
}

yaml_config::Schema HttpHandlerStatic::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<HttpHandlerBase>("src/server/handlers/http_handler_static.yaml");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END

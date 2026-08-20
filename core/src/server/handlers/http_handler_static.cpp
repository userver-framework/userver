#include <userver/server/handlers/http_handler_static.hpp>

#include <filesystem>
#include <string>
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
#include <userver/yaml_config/merge_schemas.hpp>

#include <dynamic_config/variables/USERVER_FILES_CONTENT_TYPE_MAP.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/handlers/http_handler_static.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

using Queue = concurrent::SpscQueue<std::string>;

constexpr std::size_t kReadBufferSize = 8192;
constexpr std::string_view kFileContextName = "static_file";

struct FileResponseData {
    fs::FileInfoWithDataConstPtr file;
    bool is_not_found{false};
};

std::string GetSearchPath(const http::HttpRequest& request) {
    std::string search_path;
    search_path.reserve(request.GetRequestPath().size());

    for (std::size_t i = 0; i < request.PathArgCount(); ++i) {
        auto& arg = request.GetPathArg(i);
        search_path += "/";
        search_path += arg;
    }
    return search_path;
}

FileResponseData ResolveFile(
    const fs::FsCacheClient& storage,
    const http::HttpRequest& request,
    const std::string& directory_file,
    const std::string& not_found_file
) {
    auto search_path = GetSearchPath(request);
    LOG_DEBUG() << "search_path: " << search_path;

    FileResponseData result;
    result.file = storage.TryGetFile(search_path);
    if (!result.file && !directory_file.empty()) {
        if (directory_file.front() == '/') {
            search_path = directory_file;
        } else if (search_path.empty() || search_path.back() != '/') {
            search_path += "/" + directory_file;
        } else {
            search_path += directory_file;
        }
        LOG_DEBUG() << "search_path 2: " << search_path;
        result.file = storage.TryGetFile(search_path);
    }
    if (!result.file) {
        result.file = storage.TryGetFile(not_found_file);
        result.is_not_found = true;
    }
    return result;
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

bool HttpHandlerStatic::IsStreamed(const http::HttpRequest& request, request::RequestContext& context) const {
    auto resolved = ResolveFile(storage_, request, directory_file_, not_found_file_);
    const bool
        should_stream = resolved.file && std::holds_alternative<std::filesystem::path>(resolved.file->data_or_path);
    context.SetData(std::string{kFileContextName}, std::move(resolved));
    return should_stream;
}

std::string HttpHandlerStatic::HandleRequestThrow(const http::HttpRequest& request, request::RequestContext& context)
    const {
    const auto& resolved = context.GetData<FileResponseData>(kFileContextName);

    auto& response = request.GetHttpResponse();
    if (resolved.is_not_found) {
        response.SetStatusNotFound();
    }

    if (resolved.file) {
        const auto config = config_.GetSnapshot();
        response.SetHeader(USERVER_NAMESPACE::http::headers::kExpires, std::to_string(cache_age_.count()));
        response.SetContentType(config[::dynamic_config::USERVER_FILES_CONTENT_TYPE_MAP][resolved.file->extension]);
        UASSERT(std::holds_alternative<std::string>(resolved.file->data_or_path));
        return std::get<std::string>(resolved.file->data_or_path);
    }
    response.SetStatusNotFound();
    return "File not found";
}

void HttpHandlerStatic::HandleStreamRequest(
    http::HttpRequest& request,
    request::RequestContext& context,
    http::ResponseBodyStream& stream
) const {
    const auto& resolved = context.GetData<FileResponseData>(kFileContextName);
    auto& response = request.GetHttpResponse();

    if (resolved.is_not_found) {
        response.SetStatusNotFound();
    }

    if (!resolved.file) {
        response.SetStatusNotFound();
        stream.SetEndOfHeaders();
        stream.PushBodyChunk("File not found\n", {});
        return;
    }

    UASSERT(std::holds_alternative<std::filesystem::path>(resolved.file->data_or_path));
    const auto& path = std::get<std::filesystem::path>(resolved.file->data_or_path);

    response.SetHeader(USERVER_NAMESPACE::http::headers::kExpires, std::to_string(cache_age_.count()));
    {
        const auto config = config_.GetSnapshot();
        response.SetContentType(config[::dynamic_config::USERVER_FILES_CONTENT_TYPE_MAP][resolved.file->extension]);
    }

    stream.SetEndOfHeaders();

    constexpr std::size_t kMaxQueueSize = 32;  // up to ~0.25MB of data with kReadBufferSize
    auto queue = Queue::Create(kMaxQueueSize);
    auto send_task = engine::AsyncNoTracing(fs_task_processor_, DoReadFile, path, queue->GetProducer());

    DoSendChunks(stream, queue->GetConsumer());
}

yaml_config::Schema HttpHandlerStatic::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<HttpHandlerBase>("src/server/handlers/http_handler_static.yaml");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END

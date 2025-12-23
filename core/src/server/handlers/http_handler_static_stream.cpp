#include <userver/server/handlers/http_handler_static_stream.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/fs/blocking/c_file.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/utils/async.hpp>

#include <dynamic_config/variables/USERVER_FILES_CONTENT_TYPE_MAP.hpp>
#include <userver/engine/async.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/handlers/http_handler_static_stream.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

using Queue = concurrent::StringStreamQueue;

std::string GetNormalizeDirectory(std::string_view dir) {
    auto slice = dir.size();
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto it = dir.rbegin(); it != dir.rend(); ++it) {
        if (*it == '/') {
            --slice;
        } else {
            break;
        }
    }
    return std::string{dir.data(), slice};
}
constexpr std::size_t kDefaultReadBufferSize = 8192;

void DoSendChunks(http::ResponseBodyStream& stream, Queue::Consumer consumer) {
    std::string data;
    while (!engine::current_task::ShouldCancel() && consumer.Pop(data)) {
        stream.PushBodyChunk(std::move(data), {});
    }
}

void DoReadFile(fs::blocking::CFile& file, Queue::Producer producer, std::size_t buffer_size) {
    std::string buf;

    while (!engine::current_task::ShouldCancel()) {
        buf.resize(buffer_size);
        const auto read_bytes = file.Read(buf.data(), buf.size());
        if (!read_bytes) {
            if (!std::feof(file.GetNative())) {
                LOG_INFO() << "Failed to read data";
            }
            return;
        }
        if (read_bytes != buf.size()) {
            buf.resize(read_bytes);
        }
        if (!producer.Push(std::move(buf))) {
            return;
        }
    }
}
fs::blocking::CFile SafeOpen(std::string_view path) {
    boost::system::error_code ec;
    if (const auto is_directory = boost::filesystem::is_directory(path, ec); ec || is_directory) {
        return {};
    }
    const auto fptr = std::fopen(path.data(), "rb");
    if (!fptr) {
        return {};
    }
    return fs::blocking::CFile{fptr};
}
fs::blocking::CFile AsyncSafeOpen(engine::TaskProcessor& task_processor, std::string_view path) {
    return engine::AsyncNoTracing(task_processor, &SafeOpen, std::cref(path)).Get();
}
bool IsValidaPath(std::string_view path) { return path.find("..") == std::string_view::npos; }

}  // namespace

HttpHandlerStaticStream::HttpHandlerStaticStream(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : HttpHandlerBase(config, context),
      config_(context.FindComponent<components::DynamicConfig>().GetSource()),
      base_dir_(GetNormalizeDirectory(config["dir"].As<std::string>())),
      buffer_size_(config["buffer-size"].As<std::size_t>(kDefaultReadBufferSize)),
      directory_file_(config["directory-file"].As<std::string>("index.html")),
      not_found_file_(config["not-found-file"].As<std::string>("/404.html")),
      fs_task_processor_(GetFsTaskProcessor(config, context)) {
    if (!HttpHandlerBase::IsStreamed()) {
        throw ClientError(
            HandlerErrorCode::kInvalidUsage,
            InternalMessage{fmt::format("response-body-stream must be true for {}", HttpHandlerBase::HandlerName())}
        );
    }
}

void HttpHandlerStaticStream::HandleStreamRequest(
    http::HttpRequest& request,
    request::RequestContext& /*context*/,
    http::ResponseBodyStream& stream
) const {
    std::string search_path;
    search_path.reserve(request.GetRequestPath().size());

    for (std::size_t i = 0; i < request.PathArgCount(); ++i) {
        auto& arg = request.GetPathArg(i);
        search_path += "/";
        search_path += arg;
    }

    auto& response = request.GetHttpResponse();

    const auto is_valid_path = IsValidaPath(search_path);

    LOG_DEBUG() << "search_path: " << search_path;

    auto full_path = base_dir_ + '/' + search_path;

    auto file = is_valid_path ? AsyncSafeOpen(fs_task_processor_, full_path) : fs::blocking::CFile{};
    if (file.IsOpen()) {
        response.SetStatusOk();
    }
    if (is_valid_path && !file.IsOpen() && !directory_file_.empty()) {
        if (directory_file_.front() == '/') {
            search_path = directory_file_;
        } else if (search_path.empty() || search_path[search_path.size() - 1] != '/') {
            search_path += "/" + directory_file_;
        } else {
            search_path += directory_file_;
        }
        full_path = base_dir_ + '/' + search_path;
        LOG_DEBUG() << "search_path 2: " << search_path;
        file = AsyncSafeOpen(fs_task_processor_, full_path);
    }
    if (!file.IsOpen()) {
        full_path = base_dir_ + '/' + not_found_file_;
        file = AsyncSafeOpen(fs_task_processor_, full_path);
        response.SetStatusNotFound();
    }

    if (!file.IsOpen()) {
        response.SetStatusNotFound();
        stream.SetEndOfHeaders();
        stream.PushBodyChunk("File not found\n", {});
        return;
    }
    const auto config = config_.GetSnapshot();

    const auto extension = boost::filesystem::path(full_path).extension().string();
    // LOG_DEBUG() ;
    const auto content_type = config[::dynamic_config::USERVER_FILES_CONTENT_TYPE_MAP][extension];
    LOG_DEBUG() << "extension: " << extension << ", " << "content_type: " << content_type;
    response.SetContentType(content_type);
    stream.SetEndOfHeaders();

    auto queue = Queue::Create();
    auto send_task =
        utils::Async(fs_task_processor_, "read", DoReadFile, std::ref(file), queue->GetProducer(), buffer_size_);

    DoSendChunks(stream, queue->GetConsumer());
}

yaml_config::Schema HttpHandlerStaticStream::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        HttpHandlerBase>("src/server/handlers/http_handler_static_stream.yaml");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END

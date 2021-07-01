#include <components/minimal_server_component_list.hpp>
#include <components/run.hpp>
#include <fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents

#include <clients/http/component.hpp>

/// [Flatbuf service sample - component]
#include <server/handlers/http_handler_flatbuf_base.hpp>
#include "flatbuffer_schema_generated.h"

namespace samples::fbs_handle {

class FbsSumEcho final
    : public server::handlers::HttpHandlerFlatbufBase<fbs::SampleRequest,
                                                      fbs::SampleResponse> {
 public:
  // `kName` must match component name in config.yaml
  static constexpr const char* kName = "handler-fbs-sample";

  // Component is valid after construction and is able to accept requests
  FbsSumEcho(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : HttpHandlerFlatbufBase(config, context) {}

  const std::string& HandlerName() const override {
    static const std::string kHandlerName = kName;
    return kHandlerName;
  }

  fbs::SampleResponse::NativeTableType HandleRequestFlatbufThrow(
      const server::http::HttpRequest& /*request*/,
      const fbs::SampleRequest::NativeTableType& fbs_request,
      server::request::RequestContext&) const override {
    fbs::SampleResponse::NativeTableType res;
    res.sum = fbs_request.arg1 + fbs_request.arg2;
    res.echo = fbs_request.data;
    return res;
  }
};

}  // namespace samples::fbs_handle
/// [Flatbuf service sample - component]

namespace samples::fbs_request {

/// [Flatbuf service sample - http component]
class FbsRequest final : public components::LoggableComponentBase {
 public:
  static constexpr auto kName = "fbs-request";

  FbsRequest(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : LoggableComponentBase(config, context),
        http_client_{
            context.FindComponent<components::HttpClient>().GetHttpClient()},
        task_{utils::Async("requests", [this]() { KeepRequesting(); })} {}

  void KeepRequesting() const;

 private:
  clients::http::Client& http_client_;
  engine::TaskWithResult<void> task_;
};
/// [Flatbuf service sample - http component]

/// [Flatbuf service sample - request]
void FbsRequest::KeepRequesting() const {
  // Fill the payload data
  fbs::SampleRequest::NativeTableType payload;
  payload.arg1 = 20;
  payload.arg2 = 22;
  payload.data = "Hello word";

  // Serialize the payload into a std::string
  flatbuffers::FlatBufferBuilder fbb;
  auto ret_fbb = fbs::SampleRequest::Pack(fbb, &payload);
  fbb.Finish(ret_fbb);
  std::string data(reinterpret_cast<const char*>(fbb.GetBufferPointer()),
                   fbb.GetSize());

  // Send it
  const auto response = http_client_.CreateRequest()
                            ->post("http://localhost:8084/fbs", std::move(data))
                            ->timeout(std::chrono::seconds(1))
                            ->retry(10)
                            ->perform();

  // Response code should be 200 (use proper error handling in real code!)
  UASSERT_MSG(response->IsOk(), "Sample should work well in tests");

  // Verify and deserialize response
  const auto body = response->body_view();
  const auto* response_fb =
      flatbuffers::GetRoot<fbs::SampleResponse>(body.data());
  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(body.data()),
                                 body.size());
  UASSERT_MSG(response_fb->Verify(verifier), "Broken flatbuf in sample");
  fbs::SampleResponse::NativeTableType result;
  response_fb->UnPackTo(&result);

  // Make sure that the response is the expected one for sample
  UASSERT_MSG(result.sum == 42, "Sample should work well in tests");
  UASSERT_MSG(result.echo == payload.data, "Sample should work well in tests");
}
/// [Flatbuf service sample - request]

}  // namespace samples::fbs_request

// Runtime config values to init the service.
constexpr std::string_view kRuntimeConfig = R"~({
  "HTTP_CLIENT_CONNECTION_POOL_SIZE": 1000,
  "HTTP_CLIENT_CONNECT_THROTTLE": {
    "max-size": 100,
    "token-update-interval-ms": 0
  },
  "HTTP_CLIENT_ENFORCE_TASK_DEADLINE": {
    "cancel-request": false,
    "update-timeout": false
  },

  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_LOG_REQUEST_HEADERS": false,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_HTTP_PROXY": "",
  "USERVER_TASK_PROCESSOR_QOS": {
    "default-service": {
      "default-task-processor": {
        "wait_queue_overload": {
          "action": "ignore",
          "length_limit": 5000,
          "time_limit_us": 3000
        }
      }
    }
  },
  "USERVER_CACHES": {},
  "USERVER_LRU_CACHES": {},
  "USERVER_DUMPS": {}
})~";

// Not a good path for production ready service
const auto kTmpDir = fs::blocking::TempDirectory::Create();
const std::string kRuntimeConfingPath =
    kTmpDir.GetPath() + "/runtime_config.json";

// clang-format off
const std::string kStaticConfig = R"~(
components_manager:
    coro_pool:
        initial_size: 500             # Preallocate 500 coroutines at startup.
        max_size: 1000                # Do not keep more than 1000 preallocated coroutines.

    task_processors:                  # Task processor is an executor for coroutine tasks

        main-task-processor:          # Make a task processor for CPU-bound couroutine tasks.
            worker_threads: 4         # Process tasks in 4 threads.
            thread_name: main-worker  # OS will show the threads of this task processor with 'main-worker' prefix.

        fs-task-processor:            # Make a separate task processor for filesystem bound tasks.
            thread_name: fs-worker
            worker_threads: 4

    default_task_processor: main-task-processor

    components:                       # Configuring components that were registered via component_list

        server:
            listener:                 # configuring the main listening socket...
                port: 8084            # ...to listen on this port and...
                task_processor: main-task-processor    # ...process incomming requests on this task processor.
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stdout'
                    level: debug
                    overflow_behavior: discard  # Drop logs if the system is too busy to write them down.

        tracer:                           # Component that helps to trace execution times and requests in logs.
            service-name: fbs-service

        taxi-config:                      # Runtime config options. Just loading those from file.
            fs-cache-path: )~" + kRuntimeConfingPath + R"~(
            fs-task-processor: fs-task-processor
        manager-controller:
        statistics-storage:
        auth-checker-settings:

        handler-fbs-sample:
            path: /fbs                  # Registering handler by URL '/fbs'.
            task_processor: main-task-processor  # Run it on CPU bound task processor

        fbs-request:
        http-client:                      # Component to do HTTP requests
            fs-task-processor: fs-task-processor
            user-agent: 'fbs-service 1.0'    # Set 'User-Agent' header to 'fbs-service 1.0'.
)~";
// clang-format on

int main() {
  // Use a proper persistent file in production with manually filled values!
  fs::blocking::RewriteFileContents(kRuntimeConfingPath, kRuntimeConfig);

  auto component_list = components::MinimalServerComponentList()        //
                            .Append<samples::fbs_handle::FbsSumEcho>()  //

                            .Append<components::HttpClient>()             //
                            .Append<samples::fbs_request::FbsRequest>();  //
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}

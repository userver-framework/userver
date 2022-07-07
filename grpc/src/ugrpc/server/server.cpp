#include <userver/ugrpc/server/server.hpp>

#include <algorithm>
#include <exception>
#include <memory>
#include <optional>
#include <vector>

#include <fmt/format.h>
#include <grpcpp/server.h>

#include <userver/engine/mutex.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/impl/logging.hpp>
#include <ugrpc/impl/to_string.hpp>
#include <ugrpc/server/impl/queue_holder.hpp>
#include <userver/ugrpc/server/impl/service_worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace {

bool AreServicesUnique(
    const std::vector<std::unique_ptr<impl::ServiceWorker>>& workers) {
  std::vector<std::string_view> names;
  names.reserve(workers.size());
  for (const auto& worker : workers) {
    names.push_back(worker->GetMetadata().service_full_name);
  }
  std::sort(names.begin(), names.end());
  return std::adjacent_find(names.begin(), names.end()) == names.end();
}

}  // namespace

ServerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<ServerConfig>) {
  ServerConfig config;
  config.port = value["port"].As<std::optional<int>>();
  config.native_log_level =
      value["native-log-level"].As<logging::Level>(logging::Level::kError);
  return config;
}

class Server::Impl final {
 public:
  explicit Impl(ServerConfig&& config,
                utils::statistics::Storage& statistics_storage);
  ~Impl();

  void AddService(ServiceBase& service, engine::TaskProcessor& task_processor);

  void WithServerBuilder(SetupHook&& setup);

  grpc::CompletionQueue& GetCompletionQueue() noexcept;

  void Start();

  int GetPort() const noexcept;

  void Stop() noexcept;

  void StopDebug() noexcept;

 private:
  enum class State {
    kConfiguration,
    kActive,
    kStopped,
  };

  void AddListeningPort(int port);

  void DoStart();

  State state_{State::kConfiguration};
  std::optional<grpc::ServerBuilder> server_builder_;
  std::optional<int> port_;
  std::vector<std::unique_ptr<impl::ServiceWorker>> service_workers_;
  std::optional<impl::QueueHolder> queue_;
  std::unique_ptr<grpc::Server> server_;
  engine::Mutex configuration_mutex_;

  utils::statistics::Storage& statistics_storage_;
};

Server::Impl::Impl(ServerConfig&& config,
                   utils::statistics::Storage& statistics_storage)
    : statistics_storage_(statistics_storage) {
  LOG_INFO() << "Configuring the gRPC server";
  ugrpc::impl::SetupNativeLogging();
  ugrpc::impl::UpdateNativeLogLevel(config.native_log_level);
  server_builder_.emplace();
  queue_.emplace(server_builder_->AddCompletionQueue());
  if (config.port) AddListeningPort(*config.port);
}

Server::Impl::~Impl() {
  if (state_ == State::kActive) {
    LOG_DEBUG() << "Stopping the gRPC server automatically. When using Server "
                   "outside of ServerComponent, call Stop() explicitly to "
                   "ensure that it is destroyed before services.";
    Stop();
  }
}

void Server::Impl::AddListeningPort(int port) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  UASSERT_MSG(!port_,
              "As of now, AddListeningPort can be called no more than once");
  port_ = port;
  UINVARIANT(port >= 0 && port <= 65535, "Invalid gRPC listening port");

  const auto uri = fmt::format("[::]:{}", port);
  server_builder_->AddListeningPort(ugrpc::impl::ToGrpcString(uri),
                                    grpc::InsecureServerCredentials(), &*port_);
}

void Server::Impl::AddService(ServiceBase& service,
                              engine::TaskProcessor& task_processor) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  service_workers_.push_back(service.MakeWorker(impl::ServiceSettings{
      queue_->GetQueue(), task_processor, statistics_storage_}));
}

void Server::Impl::WithServerBuilder(SetupHook&& setup) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  setup(*server_builder_);
}

grpc::CompletionQueue& Server::Impl::GetCompletionQueue() noexcept {
  UASSERT(state_ == State::kConfiguration || state_ == State::kActive);
  return queue_->GetQueue();
}

void Server::Impl::Start() {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  try {
    DoStart();
    state_ = State::kActive;
  } catch (const std::exception& ex) {
    LOG_ERROR() << "The gRPC server failed to start. " << ex;
    Stop();
    throw;
  }
}

int Server::Impl::GetPort() const noexcept {
  UASSERT(state_ == State::kActive);
  UASSERT_MSG(port_, "No port has been registered using AddListeningPort");
  return *port_;
}

void Server::Impl::Stop() noexcept {
  // Note 1: Stop must be idempotent, so that the 'Stop' invocation after a
  // 'Start' failure is optional.
  // Note 2: 'state_' remains 'kActive' while stopping, which allows clients to
  // finish their requests using 'queue_'.

  // Must shutdown server, then ServiceWorkers, then queues before anything else
  if (server_) {
    LOG_INFO() << "Stopping the gRPC server";
    server_->Shutdown();
  }
  service_workers_.clear();
  queue_.reset();
  server_.reset();

  state_ = State::kStopped;
}

void Server::Impl::StopDebug() noexcept {
  UINVARIANT(server_, "The gRPC server is not running");
  server_->Shutdown();
}

void Server::Impl::DoStart() {
  LOG_INFO() << "Starting the gRPC server";

  UASSERT_MSG(AreServicesUnique(service_workers_),
              "Multiple services have been registered "
              "for the same gRPC method");
  for (auto& worker : service_workers_) {
    server_builder_->RegisterService(&worker->GetService());
  }

  server_ = server_builder_->BuildAndStart();
  UINVARIANT(server_, "See grpcpp logs for details");
  server_builder_.reset();

  for (auto& worker : service_workers_) {
    worker->Start();
  }

  if (port_) {
    LOG_INFO() << "gRPC server started on port " << *port_;
  } else {
    LOG_INFO() << "gRPC server started without using AddListeningPort";
  }
}

Server::Server(ServerConfig&& config,
               utils::statistics::Storage& statistics_storage)
    : impl_(std::move(config), statistics_storage) {}

Server::~Server() = default;

void Server::AddService(ServiceBase& service,
                        engine::TaskProcessor& task_processor) {
  impl_->AddService(service, task_processor);
}

void Server::WithServerBuilder(SetupHook&& setup) {
  impl_->WithServerBuilder(std::move(setup));
}

grpc::CompletionQueue& Server::GetCompletionQueue() noexcept {
  return impl_->GetCompletionQueue();
}

void Server::Start() { return impl_->Start(); }

int Server::GetPort() const noexcept { return impl_->GetPort(); }

void Server::Stop() noexcept { return impl_->Stop(); }

void Server::StopDebug() noexcept { return impl_->StopDebug(); }

}  // namespace ugrpc::server

USERVER_NAMESPACE_END

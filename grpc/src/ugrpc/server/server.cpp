#include <userver/ugrpc/server/server.hpp>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include <fmt/format.h>
#include <grpcpp/ext/channelz_service_plugin.h>
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
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/impl/service_worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace {

std::optional<int> ToOptionalInt(const std::string& str) {
  char* str_end{};
  const long result = strtol(str.c_str(), &str_end, 10);
  if (str_end == str.data() + str.size() &&
      result >= std::numeric_limits<int>::min() &&
      result <= std::numeric_limits<int>::max()) {
    return result;
  } else {
    return std::nullopt;
  }
}

void ApplyChannelArgs(grpc::ServerBuilder& builder,
                      const ServerConfig& config) {
  for (const auto& [key, value] : config.channel_args) {
    if (const auto int_value = ToOptionalInt(value)) {
      builder.AddChannelArgument(ugrpc::impl::ToGrpcString(key), *int_value);
    } else {
      builder.AddChannelArgument(ugrpc::impl::ToGrpcString(key),
                                 ugrpc::impl::ToGrpcString(value));
    }
  }
}

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
  config.channel_args =
      value["channel-args"].As<decltype(config.channel_args)>({});
  config.native_log_level =
      value["native-log-level"].As<logging::Level>(logging::Level::kError);
  config.enable_channelz = value["enable-channelz"].As<bool>(false);
  config.access_log_logger_name =
      value["access-tskv-logger"].As<std::string>({});
  return config;
}

class Server::Impl final {
 public:
  explicit Impl(const ServerConfig& config,
                utils::statistics::Storage& statistics_storage,
                logging::LoggerPtr access_tskv_logger, dynamic_config::Source);
  ~Impl();

  void AddService(ServiceBase& service, engine::TaskProcessor& task_processor,
                  const Middlewares& middlewares = {});

  std::vector<std::string_view> GetServiceNames() const;

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
  mutable engine::Mutex configuration_mutex_;

  ugrpc::impl::StatisticsStorage statistics_storage_;
  const dynamic_config::Source config_source_;
  logging::LoggerPtr access_tskv_logger_;
};

Server::Impl::Impl(const ServerConfig& config,
                   utils::statistics::Storage& statistics_storage,
                   logging::LoggerPtr access_tskv_logger,
                   dynamic_config::Source config_source)
    : statistics_storage_(statistics_storage, "server"),
      config_source_(config_source),
      access_tskv_logger_(access_tskv_logger) {
  LOG_INFO() << "Configuring the gRPC server";
  ugrpc::impl::SetupNativeLogging();
  ugrpc::impl::UpdateNativeLogLevel(config.native_log_level);
  if (config.enable_channelz) {
#ifdef USERVER_DISABLE_GRPC_CHANNELZ
    UINVARIANT(false, "Channelz is disabled via USERVER_FEATURE_GRPC_CHANNELZ");
#else
    grpc::channelz::experimental::InitChannelzService();
#endif
  }
  server_builder_.emplace();
  ApplyChannelArgs(*server_builder_, config);
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
                              engine::TaskProcessor& task_processor,
                              const Middlewares& middlewares) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  service_workers_.push_back(service.MakeWorker(impl::ServiceSettings{
      queue_->GetQueue(), task_processor, statistics_storage_, middlewares,
      access_tskv_logger_, config_source_}));
}

std::vector<std::string_view> Server::Impl::GetServiceNames() const {
  std::vector<std::string_view> ret;

  std::lock_guard lock(configuration_mutex_);

  ret.reserve(service_workers_.size());
  for (const auto& worker : service_workers_) {
    ret.push_back(worker->GetMetadata().service_full_name);
  }
  return ret;
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
  service_workers_.clear();
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

Server::Server(const ServerConfig& config,
               utils::statistics::Storage& statistics_storage,
               logging::LoggerPtr access_tskv_logger,
               dynamic_config::Source config_source)
    : impl_(std::make_unique<Impl>(config, statistics_storage,
                                   access_tskv_logger, config_source)) {}

Server::~Server() = default;

void Server::AddService(ServiceBase& service,
                        engine::TaskProcessor& task_processor,
                        const Middlewares& middlewares) {
  impl_->AddService(service, task_processor, middlewares);
}

std::vector<std::string_view> Server::GetServiceNames() const {
  return impl_->GetServiceNames();
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

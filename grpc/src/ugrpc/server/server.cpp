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
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

#include <ugrpc/impl/logging.hpp>
#include <ugrpc/impl/to_string.hpp>
#include <ugrpc/server/impl/parse_config.hpp>
#include <userver/ugrpc/impl/deadline_timepoint.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/impl/queue_holder.hpp>
#include <userver/ugrpc/server/impl/service_worker.hpp>


#include <grpcpp/impl/server_builder_option.h>
#include <grpcpp/impl/server_builder_plugin.h>
#include <grpcpp/security/authorization_policy_provider.h>
#include <grpcpp/server.h>
#include <grpcpp/support/config.h>
#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace {

constexpr std::size_t kMaxSocketPathLength = 107;
constexpr std::chrono::seconds kShutdownGracePeriod{1};

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

class Server::Impl final {
 public:
  explicit Impl(ServerConfig&& config,
                utils::statistics::Storage& statistics_storage,
                dynamic_config::Source);
  ~Impl();

  void AddService(ServiceBase& service, ServiceConfig&& config);

  std::vector<std::string_view> GetServiceNames() const;

  void WithServerBuilder(SetupHook setup);

  grpc::CompletionQueue& GetCompletionQueue() noexcept;

  void Start();

  int GetPort() const noexcept;

  void StopServing() noexcept;

  void Stop() noexcept;

  std::uint64_t GetTotalRequests() const;

 private:
  enum class State {
    kConfiguration,
    kActive,
    kServingStopped,
    kStopped,
  };

  void AddListeningPort(int port);

  void AddSslConfiguration(const SslConf& conf);

  void AddListeningUnixSocket(std::string_view path);

  void DoStart();

  State state_{State::kConfiguration};
  std::optional<grpc::ServerBuilder> server_builder_;
  std::optional<int> port_;
  std::optional<int> ssl_port_;
  std::vector<std::unique_ptr<impl::ServiceWorker>> service_workers_;
  std::optional<impl::QueueHolder> queue_;
  std::unique_ptr<grpc::Server> server_;
  mutable engine::Mutex configuration_mutex_;

  ugrpc::impl::StatisticsStorage statistics_storage_;
  const dynamic_config::Source config_source_;
  logging::LoggerPtr access_tskv_logger_;
};

Server::Impl::Impl(ServerConfig&& config,
                   utils::statistics::Storage& statistics_storage,
                   dynamic_config::Source config_source)
    : statistics_storage_(statistics_storage,
                          ugrpc::impl::StatisticsDomain::kServer),
      config_source_(config_source),
      access_tskv_logger_(std::move(config.access_tskv_logger)) {
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
  queue_.emplace(static_cast<std::size_t>(config.completion_queue_num),
                 std::ref(*server_builder_));

  if (config.unix_socket_path) AddListeningUnixSocket(*config.unix_socket_path);

  if (config.port) AddListeningPort(*config.port);

  if (config.sslConf)
  {
      AddSslConfiguration(*config.sslConf);
  }
}

Server::Impl::~Impl() {
  if (state_ == State::kActive) {
    LOG_DEBUG() << "Stopping the gRPC server automatically. When using Server "
                   "outside of ServerComponent, call Stop() explicitly to "
                   "ensure that it is destroyed before services.";
    Stop();
  }
}

void Server::Impl::AddSslConfiguration(const SslConf& config)
{
    LOG_INFO() << "Configuring the gRPC server with ssl config";
    UINVARIANT(config.port >= 0 && config.port <= 65535, "Invalid gRPC listening ssl port");
    UASSERT_MSG(!ssl_port_,
                "As of now, AddSslConfiguration can be called no more than once");
    ssl_port_ = config.port;
    grpc::SslServerCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = userver::fs::blocking::ReadFileContents(config.client_root_cert);
    auto server_key = userver::fs::blocking::ReadFileContents(config.server_private_key);
    auto server_cert = userver::fs::blocking::ReadFileContents(config.server_cert);
    if(config.need_verify_client_cert) {
      ssl_opts.client_certificate_request =
          GRPC_SSL_REQUEST_CLIENT_CERTIFICATE_AND_VERIFY;
    }
    else
    {
      ssl_opts.client_certificate_request = GRPC_SSL_REQUEST_CLIENT_CERTIFICATE_BUT_DONT_VERIFY;
    }
    grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp = {server_key, server_cert};
    ssl_opts.pem_key_cert_pairs.push_back(pkcp);

    auto server_creds = SslServerCredentials(ssl_opts);
    const auto uri = fmt::format("[::]:{}", ssl_port_.value());
    LOG_INFO() << "Add ssl listening port "<<ssl_port_.value();
    server_builder_->AddListeningPort(ugrpc::impl::ToGrpcString(uri),
                                      server_creds, &*ssl_port_);
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

void Server::Impl::AddListeningUnixSocket(std::string_view path) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  UASSERT_MSG(!path.empty(), "Empty unix socket path is not allowed");
  UASSERT_MSG(path[0] == '/', "Unix socket path must be absolute");
  UINVARIANT(
      path.size() <= kMaxSocketPathLength,
      fmt::format("Unix socket path cannot contain more than {} characters",
                  kMaxSocketPathLength));

  const auto uri = fmt::format("unix:{}", path);
  server_builder_->AddListeningPort(ugrpc::impl::ToGrpcString(uri),
                                    grpc::InsecureServerCredentials());
}

void Server::Impl::AddService(ServiceBase& service, ServiceConfig&& config) {
  const std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  service_workers_.push_back(service.MakeWorker(impl::ServiceSettings{
      *queue_,
      config.task_processor,
      statistics_storage_,
      std::move(config.middlewares),
      access_tskv_logger_,
      config_source_,
      config.end_point
  }));
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

void Server::Impl::WithServerBuilder(SetupHook setup) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  setup(*server_builder_);
}

grpc::CompletionQueue& Server::Impl::GetCompletionQueue() noexcept {
  UASSERT(state_ == State::kConfiguration || state_ == State::kActive);
  // TODO: https://st.yandex-team.ru/TAXICOMMON-7612
  return *queue_->GetQueues().queues[0];
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
  // Note 2: 'state_' remains 'kActive' while stopping, which allows clients
  // to finish their requests using 'queue_'.

  // Must shutdown server, then ServiceWorkers, then queues before anything
  // else
  if (server_) {
    LOG_INFO() << "Stopping the gRPC server";
    server_->Shutdown(engine::Deadline::FromDuration(kShutdownGracePeriod));
  }
  service_workers_.clear();
  queue_.reset();
  server_.reset();

  state_ = State::kStopped;
}

void Server::Impl::StopServing() noexcept {
  UASSERT(state_ != State::kStopped);
  if (server_) {
    LOG_INFO() << "Stopping serving on the gRPC server";
    server_->Shutdown(engine::Deadline::FromDuration(kShutdownGracePeriod));
  }
  service_workers_.clear();

  state_ = State::kServingStopped;
}

std::uint64_t Server::Impl::GetTotalRequests() const {
  return statistics_storage_.GetStartedRequests();
}

void Server::Impl::DoStart() {
  LOG_INFO() << "Starting the gRPC server";

  UASSERT_MSG(AreServicesUnique(service_workers_),
              "Multiple services have been registered "
              "for the same gRPC method");
  for (auto& worker : service_workers_) {
    auto end_point = worker->EndPoint();
    if(end_point)
    {
      LOG_INFO() << "Register service to endpoint  "<<end_point.value();
      server_builder_->RegisterService(end_point.value(), &worker->GetService());
    }
    else
    {
      LOG_INFO() << "Register service wo endpoint  ";
      server_builder_->RegisterService(&worker->GetService());
    }
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
               utils::statistics::Storage& statistics_storage,
               dynamic_config::Source config_source)
    : impl_(std::make_unique<Impl>(std::move(config), statistics_storage,
                                   config_source)) {}

Server::~Server() = default;

void Server::AddService(ServiceBase& service, ServiceConfig&& config) {
  impl_->AddService(service, std::move(config));
}

std::vector<std::string_view> Server::GetServiceNames() const {
  return impl_->GetServiceNames();
}

void Server::WithServerBuilder(SetupHook setup) {
  impl_->WithServerBuilder(setup);
}

grpc::CompletionQueue& Server::GetCompletionQueue() noexcept {
  return impl_->GetCompletionQueue();
}

void Server::Start() { return impl_->Start(); }

int Server::GetPort() const noexcept { return impl_->GetPort(); }

void Server::Stop() noexcept { return impl_->Stop(); }

void Server::StopServing() noexcept { return impl_->StopServing(); }

std::uint64_t Server::GetTotalRequests() const {
  return impl_->GetTotalRequests();
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END

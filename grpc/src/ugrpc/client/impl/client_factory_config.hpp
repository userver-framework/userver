#pragma once

#include <userver/ugrpc/client/client_factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

enum class AuthType {
  kInsecure,
  kSsl,
};

/// Settings relating to the ClientFactory
struct ClientFactoryConfig final {
  AuthType auth_type{AuthType::kInsecure};
  /// The buffer containing the PEM encoding of the server root certificates. If
  /// this parameter is empty, the default roots will be used.  The default
  /// roots can be overridden using the \a GRPC_DEFAULT_SSL_ROOTS_FILE_PATH
  /// environment variable pointing to a file on the file system containing the
  /// roots.
  std::optional<std::string> pem_root_certs;
  /// The buffer containing the PEM encoding of the client's private key. This
  /// parameter can be empty if the client does not have a private key.
  std::optional<std::string> pem_private_key;
  /// The buffer containing the PEM encoding of the client's certificate chain.
  /// This parameter can be empty if the client does not have a certificate
  /// chain.
  std::optional<std::string> pem_cert_chain;

  /// Optional grpc-core channel args
  /// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  grpc::ChannelArguments channel_args{};

  /// The logging level override for the internal grpcpp library. Must be either
  /// `kDebug`, `kInfo` or `kError`.
  logging::Level native_log_level{logging::Level::kError};

  /// Number of underlying channels that will be created for every client
  /// in this factory.
  std::size_t channel_count{1};
};

ClientFactoryConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<ClientFactoryConfig>);

ClientFactorySettings MakeFactorySettings(
    impl::ClientFactoryConfig&& config,
    const storages::secdist::SecdistConfig* secdist, const testsuite::GrpcControl& testsuite_grpc);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END

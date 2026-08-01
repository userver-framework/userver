#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <ydb-cpp-sdk/client/types/credentials/credentials.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/ydb/settings.hpp>
#include <ydb-cpp-sdk/client/types/ydb.h>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace secdist {
struct DatabaseSettings;
}  // namespace secdist

struct TableSettings {
    std::uint32_t min_pool_size{10};
    std::uint32_t max_pool_size{50};
    std::uint32_t get_session_retry_limit{5};
    // Deprecated, parsed for backward compatibility but not used by execute.
    bool keep_in_query_cache{true};
    bool sync_start{true};
    // Deprecated, parsed for backward compatibility but not used by execute.
    bool use_query_client{true};
    std::optional<std::vector<double>> by_database_timings_buckets{};
    std::optional<std::vector<double>> by_query_timings_buckets{};
};

struct TopicSettings {};

struct TcpKeepaliveSettings {
    bool enabled;
    std::size_t idle_sec;
    std::size_t probe_count;
    std::size_t interval_sec;
};

struct DriverSettings {
    std::string endpoint;
    std::string database;

    std::optional<std::size_t> network_threads_num{};
    std::optional<std::size_t> client_threads_num{};

    std::optional<TcpKeepaliveSettings> tcp_keepalive{};

    std::optional<std::chrono::milliseconds> grpc_keepalive_timeout{};
    std::optional<bool> grpc_keepalive_permit_without_calls{};

    std::optional<NYdb::EGrpcCompressionAlgorithm> grpc_compression_algorithm{};
    std::optional<std::string> grpc_load_balancing_policy{};

    bool prefer_local_dc{false};
    std::optional<std::string> oauth_token;
    std::optional<std::string> iam_jwt_params;
    std::optional<std::string> secure_connection_cert;
    std::optional<std::string> user;
    std::optional<std::string> password;
    std::shared_ptr<NYdb::ICredentialsProviderFactory> credentials_provider_factory;
};

std::string_view ToString(NYdb::EGrpcCompressionAlgorithm algorithm);

TableSettings ParseTableSettings(const yaml_config::YamlConfig& dbconfig, const secdist::DatabaseSettings& dbsecdist);

DriverSettings ParseDriverSettings(
    const yaml_config::YamlConfig& dbconfig,
    const secdist::DatabaseSettings& dbsecdist,
    std::shared_ptr<NYdb::ICredentialsProviderFactory> credentials_provider_factory
);

inline constexpr int kDeadlinePropagationExperimentVersion = 1;

}  // namespace ydb::impl

USERVER_NAMESPACE_END

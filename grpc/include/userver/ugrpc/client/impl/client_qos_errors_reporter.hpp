#pragma once

#include <userver/ugrpc/impl/static_service_metadata.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {
struct ClientQos;
}  // namespace ugrpc::client

namespace ugrpc::client::impl {

class ClientQosErrorsReporter final {
public:
    explicit ClientQosErrorsReporter(utils::statistics::MetricsStorage& metrics_storage);

    /// @brief Validates client QOS configuration and synchronizes validity alert state.
    /// @details Performs validation using @ref ValidateClientQosMethodsExistence and:
    /// - Fires `grpc_client_qos_invalid_configuration` alert if errors are found
    /// - Stops the alert if configuration is valid
    /// Logs warnings for each detected issue.
    void ValidateAndReportClientQosErrors(
        const ClientQos& client_qos,
        std::string_view config_name,
        const ugrpc::impl::StaticServiceMetadata& metadata
    );

private:
    utils::statistics::MetricsStorage& metrics_storage_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END

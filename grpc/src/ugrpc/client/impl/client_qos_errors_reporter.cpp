#include <userver/ugrpc/client/impl/client_qos_errors_reporter.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

#include <ugrpc/client/impl/client_qos_validation.hpp>
#include <userver/alerts/source.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

const alerts::Source kGrpcClientQosInvalidConfiguration("grpc_client_qos_invalid_configuration");

void UpdateAlertState(utils::statistics::MetricsStorage& metrics_storage, QosValidationResult validation_result) {
    switch (validation_result) {
        case QosValidationResult::kOk:
            kGrpcClientQosInvalidConfiguration.StopAlertNow(metrics_storage);
            break;
        case QosValidationResult::kHasErrors:
            kGrpcClientQosInvalidConfiguration.FireAlert(metrics_storage, alerts::Source::kInfiniteDuration);
            break;
        default:
            UINVARIANT(false, "Unexpected QosValidationResult");
            break;
    }
}

}  // namespace

ClientQosErrorsReporter::ClientQosErrorsReporter(utils::statistics::MetricsStorage& metrics_storage)
    : metrics_storage_(metrics_storage)
{}

void ClientQosErrorsReporter::ValidateAndReportClientQosErrors(
    const ClientQos& client_qos,
    std::string_view config_name,
    const ugrpc::impl::StaticServiceMetadata& metadata
) {
    auto log_error = [&](std::string_view method_path, RpcPathValidationError error) {
        if (error == RpcPathValidationError::kNotFound) {
            LOG_WARNING(
                "Method '{}' specified in '{}' config does not exist in service '{}'. This QOS "
                "configuration will be ignored. Available methods: {}",
                method_path,
                config_name,
                metadata.service_full_name,
                boost::algorithm::join(
                    boost::copy_range<std::vector<std::string>>(
                        boost::irange(static_cast<std::size_t>(0), GetMethodsCount(metadata)) |
                        boost::adaptors::transformed([&](std::size_t i) { return GetMethodFullName(metadata, i); })
                    ),
                    ", "
                )
            );
        } else {
            LOG_WARNING(
                "Invalid RPC method path format in '{}' config: '{}'. Reason: {}. Expected format: "
                "'path.to.ServiceName/MethodName' (without leading slash). This QOS configuration will be "
                "ignored.",
                config_name,
                method_path,
                ToString(error)
            );
        }
    };

    UpdateAlertState(metrics_storage_, ValidateClientQosMethodsExistence(client_qos, metadata, std::move(log_error)));
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END

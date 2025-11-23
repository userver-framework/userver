#include <userver/ugrpc/rich_status.hpp>

#include <google/protobuf/any.pb.h>
#include <google/rpc/error_details.pb.h>

#include <userver/ugrpc/datetime_utils.hpp>
#include <userver/ugrpc/status_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

google::rpc::Status GrpcStatusToGoogleRpcStatus(const grpc::Status& grpc_status) {
    const auto grpc_status_opt = ugrpc::ToGoogleRpcStatus(grpc_status);
    if (grpc_status_opt) {
        return *grpc_status_opt;
    } else {
        auto google_status = google::rpc::Status{};
        google_status.set_code(static_cast<int>(grpc_status.error_code()));
        google_status.set_message(grpc_status.error_message());
        return google_status;
    }
}

// analogue of std::forward_like from c++23
template <typename TOwner, typename TMember>
decltype(auto) ForwardLike(TMember& member) {
    if constexpr (std::is_lvalue_reference_v<TOwner>) {
        return member;
    } else {
        return std::move(member);
    }
}

template <typename TErrorInfo>
google::rpc::ErrorInfo ErrorInfoToGoogleErrorDetailImpl(TErrorInfo&& error_info) {
    google::rpc::ErrorInfo pb_error_info;
    pb_error_info.set_reason(std::forward<TErrorInfo>(error_info).reason);
    pb_error_info.set_domain(std::forward<TErrorInfo>(error_info).domain);
    auto* pb_metadata = pb_error_info.mutable_metadata();
    for (auto&& [key, value] : error_info.metadata) {
        (*pb_metadata)[ForwardLike<TErrorInfo>(key)] = ForwardLike<TErrorInfo>(value);
    }
    return pb_error_info;
}

template <typename TDebugInfo>
google::rpc::DebugInfo DebugInfoToGoogleErrorDetailImpl(TDebugInfo&& debug_info) {
    google::rpc::DebugInfo pb_debug_info;
    pb_debug_info.mutable_stack_entries()->Reserve(debug_info.stack_entries.size());
    for (auto&& entry : debug_info.stack_entries) {
        pb_debug_info.add_stack_entries(ForwardLike<TDebugInfo>(entry));
    }
    pb_debug_info.set_detail(std::forward<TDebugInfo>(debug_info).detail);
    return pb_debug_info;
}

template <typename TQuotaFailure>
google::rpc::QuotaFailure QuotaFailureToGoogleErrorDetailImpl(TQuotaFailure&& quota_failure) {
    google::rpc::QuotaFailure pb_quota_failure;
    pb_quota_failure.mutable_violations()->Reserve(quota_failure.violations.size());
    for (auto&& violation : quota_failure.violations) {
        auto* pb_violation = pb_quota_failure.add_violations();
        pb_violation->set_subject(ForwardLike<TQuotaFailure>(violation.subject));
        pb_violation->set_description(ForwardLike<TQuotaFailure>(violation.description));
    }
    return pb_quota_failure;
}

template <typename TPreconditionFailure>
google::rpc::PreconditionFailure PreconditionFailureToGoogleErrorDetailImpl(TPreconditionFailure&& precondition_failure
) {
    google::rpc::PreconditionFailure pb_precondition_failure;
    pb_precondition_failure.mutable_violations()->Reserve(precondition_failure.violations.size());
    for (auto&& violation : precondition_failure.violations) {
        auto* pb_violation = pb_precondition_failure.add_violations();
        pb_violation->set_type(ForwardLike<TPreconditionFailure>(violation.type));
        pb_violation->set_subject(ForwardLike<TPreconditionFailure>(violation.subject));
        pb_violation->set_description(ForwardLike<TPreconditionFailure>(violation.description));
    }
    return pb_precondition_failure;
}

template <typename TBadRequest>
google::rpc::BadRequest BadRequestToGoogleErrorDetailImpl(TBadRequest&& bad_request) {
    google::rpc::BadRequest pb_bad_request;
    pb_bad_request.mutable_field_violations()->Reserve(bad_request.field_violations.size());
    for (auto&& violation : bad_request.field_violations) {
        auto* pb_violation = pb_bad_request.add_field_violations();
        pb_violation->set_field(ForwardLike<TBadRequest>(violation.field));
        pb_violation->set_description(ForwardLike<TBadRequest>(violation.description));
    }
    return pb_bad_request;
}

template <typename TRequestInfo>
google::rpc::RequestInfo RequestInfoToGoogleErrorDetailImpl(TRequestInfo&& request_info) {
    google::rpc::RequestInfo pb_request_info;
    pb_request_info.set_request_id(std::forward<TRequestInfo>(request_info).request_id);
    pb_request_info.set_serving_data(std::forward<TRequestInfo>(request_info).serving_data);
    return pb_request_info;
}

template <typename TResourceInfo>
google::rpc::ResourceInfo ResourceInfoToGoogleErrorDetailImpl(TResourceInfo&& resource_info) {
    google::rpc::ResourceInfo pb_resource_info;
    pb_resource_info.set_resource_type(std::forward<TResourceInfo>(resource_info).resource_type);
    pb_resource_info.set_resource_name(std::forward<TResourceInfo>(resource_info).resource_name);
    pb_resource_info.set_owner(std::forward<TResourceInfo>(resource_info).owner);
    pb_resource_info.set_description(std::forward<TResourceInfo>(resource_info).description);
    return pb_resource_info;
}

template <typename THelp>
google::rpc::Help HelpToGoogleErrorDetailImpl(THelp&& help) {
    google::rpc::Help pb_help;
    pb_help.mutable_links()->Reserve(help.links.size());
    for (auto&& link : help.links) {
        auto* pb_link = pb_help.add_links();
        pb_link->set_description(ForwardLike<THelp>(link.description));
        pb_link->set_url(ForwardLike<THelp>(link.url));
    }
    return pb_help;
}

template <typename TLocalizedMessage>
google::rpc::LocalizedMessage LocalizedMessageToGoogleErrorDetailImpl(TLocalizedMessage&& localized_message) {
    google::rpc::LocalizedMessage pb_localized_message;
    pb_localized_message.set_locale(std::forward<TLocalizedMessage>(localized_message).locale);
    pb_localized_message.set_message(std::forward<TLocalizedMessage>(localized_message).message);
    return pb_localized_message;
}

template <typename T>
std::optional<T> TryUnpackImpl(const ::google::protobuf::Any& any) {
    T proto_message;
    if (any.UnpackTo(&proto_message)) {
        return proto_message;
    }
    return std::nullopt;
}

}  // namespace

RichStatus::RichStatus(const grpc::Status& grpc_status)
    : google_status_{GrpcStatusToGoogleRpcStatus(grpc_status)}
{}

grpc::Status RichStatus::ToGrpcStatus() const { return ugrpc::ToGrpcStatus(google_status_); }

google::rpc::ErrorInfo ErrorInfo::ToGoogleErrorDetail() const& { return ErrorInfoToGoogleErrorDetailImpl(*this); }

google::rpc::ErrorInfo ErrorInfo::ToGoogleErrorDetail() && {
    return ErrorInfoToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<ErrorInfo> ErrorInfo::TryUnpack(const ::google::protobuf::Any& any) {
    const auto pb_error_info_opt = TryUnpackImpl<google::rpc::ErrorInfo>(any);
    if (!pb_error_info_opt) {
        return std::nullopt;
    }

    return ErrorInfo{
        pb_error_info_opt->reason(),
        pb_error_info_opt->domain(),
        {pb_error_info_opt->metadata().begin(), pb_error_info_opt->metadata().end()},
    };
}

google::rpc::RetryInfo RetryInfo::ToGoogleErrorDetail() const {
    google::rpc::RetryInfo pb_retry_info;
    *pb_retry_info.mutable_retry_delay() = ToProtoDuration(retry_delay);
    return pb_retry_info;
}

std::optional<RetryInfo> RetryInfo::TryUnpack(const ::google::protobuf::Any& any) {
    const auto pb_retry_info_opt = TryUnpackImpl<google::rpc::RetryInfo>(any);
    if (!pb_retry_info_opt) {
        return std::nullopt;
    }

    return RetryInfo{ToDuration<std::chrono::nanoseconds>(pb_retry_info_opt->retry_delay())};
}

google::rpc::DebugInfo DebugInfo::ToGoogleErrorDetail() const& { return DebugInfoToGoogleErrorDetailImpl(*this); }

google::rpc::DebugInfo DebugInfo::ToGoogleErrorDetail() && {
    return DebugInfoToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<DebugInfo> DebugInfo::TryUnpack(const ::google::protobuf::Any& any) {
    const auto pb_debug_info_opt = TryUnpackImpl<google::rpc::DebugInfo>(any);
    if (!pb_debug_info_opt) {
        return std::nullopt;
    }

    return DebugInfo{
        {pb_debug_info_opt->stack_entries().begin(), pb_debug_info_opt->stack_entries().end()},
        pb_debug_info_opt->detail(),
    };
}

google::rpc::QuotaFailure QuotaFailure::ToGoogleErrorDetail() const& {
    return QuotaFailureToGoogleErrorDetailImpl(*this);
}

google::rpc::QuotaFailure QuotaFailure::ToGoogleErrorDetail() && {
    return QuotaFailureToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<QuotaFailure> QuotaFailure::TryUnpack(const ::google::protobuf::Any& any) {
    const auto pb_quota_failure_opt = TryUnpackImpl<google::rpc::QuotaFailure>(any);
    if (!pb_quota_failure_opt) {
        return std::nullopt;
    }

    const auto& pb_violations = pb_quota_failure_opt->violations();

    QuotaFailure result;
    result.violations.reserve(pb_violations.size());
    for (const auto& pb_violation : pb_violations) {
        result.violations.push_back(QuotaViolation{pb_violation.subject(), pb_violation.description()});
    }
    return result;
}

google::rpc::PreconditionFailure PreconditionFailure::ToGoogleErrorDetail() const& {
    return PreconditionFailureToGoogleErrorDetailImpl(*this);
}

google::rpc::PreconditionFailure PreconditionFailure::ToGoogleErrorDetail() && {
    return PreconditionFailureToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<PreconditionFailure> PreconditionFailure::TryUnpack(const ::google::protobuf::Any& any) {
    auto pb_precondition_failure_opt = TryUnpackImpl<google::rpc::PreconditionFailure>(any);
    if (!pb_precondition_failure_opt) {
        return std::nullopt;
    }

    const auto& pb_violations = pb_precondition_failure_opt->violations();

    PreconditionFailure result;
    result.violations.reserve(pb_violations.size());
    for (const auto& pb_violation : pb_violations) {
        result.violations
            .push_back(PreconditionViolation{pb_violation.type(), pb_violation.subject(), pb_violation.description()});
    }
    return result;
}

google::rpc::BadRequest BadRequest::ToGoogleErrorDetail() const& { return BadRequestToGoogleErrorDetailImpl(*this); }

google::rpc::BadRequest BadRequest::ToGoogleErrorDetail() && {
    return BadRequestToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<BadRequest> BadRequest::TryUnpack(const ::google::protobuf::Any& any) {
    auto pb_bad_request_opt = TryUnpackImpl<google::rpc::BadRequest>(any);
    if (!pb_bad_request_opt) {
        return std::nullopt;
    }

    const auto& pb_field_violations = pb_bad_request_opt->field_violations();

    BadRequest result;
    result.field_violations.reserve(pb_field_violations.size());
    for (const auto& pb_violation : pb_field_violations) {
        result.field_violations.push_back(FieldViolation{pb_violation.field(), pb_violation.description()});
    }
    return result;
}

google::rpc::RequestInfo RequestInfo::ToGoogleErrorDetail() const& { return RequestInfoToGoogleErrorDetailImpl(*this); }

google::rpc::RequestInfo RequestInfo::ToGoogleErrorDetail() && {
    return RequestInfoToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<RequestInfo> RequestInfo::TryUnpack(const ::google::protobuf::Any& any) {
    auto pb_request_info_opt = TryUnpackImpl<google::rpc::RequestInfo>(any);
    if (!pb_request_info_opt) {
        return std::nullopt;
    }

    return RequestInfo{
        pb_request_info_opt->request_id(),
        pb_request_info_opt->serving_data(),
    };
}

google::rpc::ResourceInfo ResourceInfo::ToGoogleErrorDetail() const& {
    return ResourceInfoToGoogleErrorDetailImpl(*this);
}

google::rpc::ResourceInfo ResourceInfo::ToGoogleErrorDetail() && {
    return ResourceInfoToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<ResourceInfo> ResourceInfo::TryUnpack(const ::google::protobuf::Any& any) {
    auto pb_resource_info_opt = TryUnpackImpl<google::rpc::ResourceInfo>(any);
    if (!pb_resource_info_opt) {
        return std::nullopt;
    }

    return ResourceInfo{
        pb_resource_info_opt->resource_type(),
        pb_resource_info_opt->resource_name(),
        pb_resource_info_opt->owner(),
        pb_resource_info_opt->description(),
    };
}

google::rpc::Help Help::ToGoogleErrorDetail() const& { return HelpToGoogleErrorDetailImpl(*this); }

google::rpc::Help Help::ToGoogleErrorDetail() && { return HelpToGoogleErrorDetailImpl(std::move(*this)); }

std::optional<Help> Help::TryUnpack(const ::google::protobuf::Any& any) {
    auto pb_help_opt = TryUnpackImpl<google::rpc::Help>(any);
    if (!pb_help_opt) {
        return std::nullopt;
    }

    const auto& pb_links = pb_help_opt->links();

    Help result;
    result.links.reserve(pb_links.size());
    for (const auto& pb_link : pb_links) {
        result.links.push_back(HelpLink{pb_link.description(), pb_link.url()});
    }
    return result;
}

google::rpc::LocalizedMessage LocalizedMessage::ToGoogleErrorDetail() const& {
    return LocalizedMessageToGoogleErrorDetailImpl(*this);
}

google::rpc::LocalizedMessage LocalizedMessage::ToGoogleErrorDetail() && {
    return LocalizedMessageToGoogleErrorDetailImpl(std::move(*this));
}

std::optional<LocalizedMessage> LocalizedMessage::TryUnpack(const ::google::protobuf::Any& any) {
    auto pb_localized_message_opt = TryUnpackImpl<google::rpc::LocalizedMessage>(any);
    if (!pb_localized_message_opt) {
        return std::nullopt;
    }

    return LocalizedMessage{
        pb_localized_message_opt->locale(),
        pb_localized_message_opt->message(),
    };
}

}  // namespace ugrpc

USERVER_NAMESPACE_END

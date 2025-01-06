#pragma once

/// @file userver/ugrpc/server/result.hpp
/// @brief @copybrief ugrpc::server::Result

#include <optional>
#include <variant>

#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Result type for service handlers (non server-streaming)
///
/// Provides a way to return either Response or grpc::Status
template <typename Response>
class Result {
public:
    /// Construct instance from Response, imply success status
    /*implicit*/ Result(Response&& response) : result_{std::move(response)} {}

    /// Construct instance from grpc::Status, only error status allowed
    /*implicit*/ Result(grpc::Status&& status) : result_{std::move(status)} {
        UINVARIANT(!GetErrorStatus().ok(), "Only error status allowed");
    }

    /// Construct instance from grpc::Status, only error status allowed
    /*implicit*/ Result(const grpc::Status& status) : result_{status} {
        UINVARIANT(!GetErrorStatus().ok(), "Only error status allowed");
    }

    /// @cond
    bool IsSuccess() const { return std::holds_alternative<Response>(result_); }

    Response&& ExtractResponse() && {
        UINVARIANT(IsSuccess(), "ExtractResponse allowed only in success state");
        return std::get<Response>(std::move(result_));
    }

    const Response& GetResponse() const {
        UINVARIANT(IsSuccess(), "GetResponse allowed only in success state");
        return std::get<Response>(result_);
    }

    const grpc::Status& GetErrorStatus() const {
        UINVARIANT(!IsSuccess(), "GetErrorStatus allowed only in error state");
        return std::get<grpc::Status>(result_);
    }
    /// @endcond

private:
    std::variant<Response, grpc::Status> result_;
};

/// @brief Special result type for server-streaming service handlers
template <typename Response>
class StreamingResult final {
public:
    /// Construct instance from grpc::Status
    /*implicit*/ StreamingResult(grpc::Status&& status) : status_{std::move(status)} {}

    /// Construct instance from grpc::Status
    /*implicit*/ StreamingResult(const grpc::Status& status) : status_{status} {}

    /// Construct instance with last response
    ///
    /// Allows perform writing last response and coalesce it with status in a
    /// single step.
    /*implicit*/ StreamingResult(Response&& last_response) : last_response_(std::move(last_response)) {}

    /// @cond
    bool HasLastResponse() const { return last_response_.has_value(); }

    Response&& ExtractLastResponse() && { return std::move(last_response_).value(); }

    const Response& GetLastResponse() const { return last_response_.value(); }

    const grpc::Status& GetStatus() const { return status_; }
    /// @endcond

private:
    std::optional<Response> last_response_;
    grpc::Status status_{grpc::Status::OK};
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END

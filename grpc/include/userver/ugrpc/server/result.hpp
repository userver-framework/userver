#pragma once

/// @file userver/ugrpc/server/result.hpp
/// @brief @copybrief ugrpc::server::Result

#include <optional>
#include <variant>

#include <grpcpp/support/status.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Result type for service handlers (non server-streaming).
///
/// Provides a way to return one of:
/// 1. `Response` + `grpc::Status::OK`;
/// 2. error `grpc::Status`.
///
/// For server-streaming RPCs, see @ref ugrpc::server::StreamingResult.
template <typename Response>
class Result {
public:
    /// Construct from `Response`, implies `OK` status.
    /*implicit*/ Result(Response&& response) : result_{std::move(response)} {}

    /// Construct from `grpc::Status`, only error status is allowed.
    /*implicit*/ Result(grpc::Status&& status) : result_{std::move(status)} {
        UINVARIANT(!GetErrorStatus().ok(), "Only error status is allowed, for OK status a response should be provided");
    }

    /// Construct from `grpc::Status`, only error status is allowed.
    /*implicit*/ Result(const grpc::Status& status) : result_{status} {
        UINVARIANT(!GetErrorStatus().ok(), "Only error status is allowed, for OK status a response should be provided");
    }

    /// @returns `true` iff the `Result` contains `Response`, as opposed to an error status.
    bool IsSuccess() const { return std::holds_alternative<Response>(result_); }

    /// @returns the contained error status.
    /// @pre `IsSuccess() == false`.
    grpc::Status&& ExtractErrorStatus() {
        UINVARIANT(!IsSuccess(), "ExtractErrorStatus is only allowed in case of error status");
        return std::move(std::get<grpc::Status>(result_));
    }

    /// @returns the contained error status.
    /// @pre `IsSuccess() == false`.
    const grpc::Status& GetErrorStatus() const {
        UINVARIANT(!IsSuccess(), "GetErrorStatus is only allowed in case of error status");
        return std::get<grpc::Status>(result_);
    }

    /// @returns the contained response.
    /// @pre `IsSuccess() == true`.
    Response&& ExtractResponse() {
        UINVARIANT(IsSuccess(), "ExtractResponse is only allowed in case of OK status");
        return std::get<Response>(std::move(result_));
    }

    /// @returns the contained error status.
    /// @pre `IsSuccess() == true`.
    const Response& GetResponse() const {
        UINVARIANT(IsSuccess(), "GetResponse is only allowed in case of OK status");
        return std::get<Response>(result_);
    }

private:
    std::variant<Response, grpc::Status> result_;
};

/// @brief Special result type for server-streaming service handlers.
///
/// Provides a way to return one of:
/// 1. `grpc::Status::OK`;
/// 2. error `grpc::Status`;
/// 3. last response + `grpc::Status::OK`, sent in a single batch.
///
/// For non-server-streaming RPCs, see @ref ugrpc::server::Result.
template <typename Response>
class StreamingResult final {
public:
    /// Construct from `grpc::Status` (which can be `OK` or an error status).
    /*implicit*/ StreamingResult(grpc::Status&& status) : status_{std::move(status)} {}

    /// Construct from `grpc::Status` (which can be `OK` or an error status).
    /*implicit*/ StreamingResult(const grpc::Status& status) : status_{status} {}

    /// Construct from last response. Allows to send last response and `OK` status coalesced in a single batch.
    /*implicit*/ StreamingResult(Response&& last_response) : last_response_(std::move(last_response)) {}

    /// @returns `true` iff the `StreamingResult` contains `OK` status, possibly with a last response.
    bool IsSuccess() const { return status_.ok(); }

    /// @returns the contained status, which can be `OK` or an error status.
    grpc::Status&& ExtractStatus() { return std::move(status_); }

    /// @returns the contained status, which can be `OK` or an error status.
    const grpc::Status& GetStatus() const { return status_; }

    /// @returns `true` iff the `StreamingResult` contains last response, which implies `OK` status.
    bool HasLastResponse() const { return last_response_.has_value(); }

    /// @returns the contained last response.
    /// @pre `HasLastResponse() == true`.
    Response&& ExtractLastResponse() {
        UINVARIANT(HasLastResponse(), "There is no last response in the StreamingResult");
        return std::move(last_response_).value();
    }

    /// @returns the contained last response.
    /// @pre `HasLastResponse() == true`.
    const Response& GetLastResponse() const {
        UINVARIANT(HasLastResponse(), "There is no last response in the StreamingResult");
        return last_response_.value();
    }

private:
    std::optional<Response> last_response_;
    grpc::Status status_{grpc::Status::OK};
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END

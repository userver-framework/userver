#pragma once

/// @file userver/ugrpc/server/result.hpp
/// @brief @copybrief ugrpc::server::Result

#include <optional>
#include <variant>

#include <grpcpp/support/status.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/server/impl/message_storage.hpp>

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
    /*implicit*/ Result(Response&& response)
        : result_{std::in_place_type<impl::MessageStorage<Response>>, std::move(response)}
    {}

    /// @brief Construct a successful result that borrows an arena-backed protobuf response.
    /// @param response A protobuf message allocated on an arena.
    /// @pre The arena must remain alive and must not be reset until result consumption,
    /// serialization, and middleware hooks have finished.
    /// @returns A result referring to @a response without taking ownership.
    static Result BorrowArenaResponse(Response& response) {
        UINVARIANT(response.GetArena(), "The response must be allocated on an arena");
        return Result(std::ref(response));
    }

    /// Construct from `grpc::Status`, only error status is allowed.
    /*implicit*/ Result(grpc::Status&& status)
        : result_{std::move(status)}
    {
        UINVARIANT(!GetErrorStatus().ok(), "Only error status is allowed, for OK status a response should be provided");
    }

    /// Construct from `grpc::Status`, only error status is allowed.
    /*implicit*/ Result(const grpc::Status& status)
        : result_{status}
    {
        UINVARIANT(!GetErrorStatus().ok(), "Only error status is allowed, for OK status a response should be provided");
    }

    /// @returns `true` if the `Result` contains `Response`, as opposed to an error status.
    bool IsSuccess() const { return std::holds_alternative<impl::MessageStorage<Response>>(result_); }

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
    /// @warning Extracting an arena-backed response is usually a mistake: moving it into a standalone message
    /// copies its data off the arena. The returned reference does not extend the arena's lifetime.
    /// Prefer @ref ugrpc::server::Result::GetResponse for read-only access.
    Response&& ExtractResponse() {
        UINVARIANT(IsSuccess(), "ExtractResponse is only allowed in case of OK status");
        return std::move(std::get<impl::MessageStorage<Response>>(result_).Get());
    }

    /// @returns the contained response.
    /// @pre `IsSuccess() == true`.
    const Response& GetResponse() const {
        UINVARIANT(IsSuccess(), "GetResponse is only allowed in case of OK status");
        return std::get<impl::MessageStorage<Response>>(result_).Get();
    }

    /// @cond
    // For internal use only.
    impl::MessageStorage<Response>&& ExtractStorage(utils::impl::InternalTag) {
        return std::move(std::get<impl::MessageStorage<Response>>(result_));
    }
    /// @endcond

private:
    explicit Result(std::reference_wrapper<Response> response)
        : result_{std::in_place_type<impl::MessageStorage<Response>>, response}
    {}

    std::variant<impl::MessageStorage<Response>, grpc::Status> result_;
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
    /*implicit*/ StreamingResult(grpc::Status&& status)
        : status_{std::move(status)}
    {}

    /// Construct from `grpc::Status` (which can be `OK` or an error status).
    /*implicit*/ StreamingResult(const grpc::Status& status)
        : status_{status}
    {}

    /// Construct from last response. Allows to send last response and `OK` status coalesced in a single batch.
    /*implicit*/ StreamingResult(Response&& last_response)
        : last_response_(std::move(last_response))
    {}

    /// @brief Construct a successful result that borrows an arena-backed last response.
    /// @param response A protobuf message allocated on an arena.
    /// @pre The arena must remain alive and must not be reset until result consumption,
    /// serialization, and middleware hooks have finished.
    /// @returns A result referring to @a response without taking ownership.
    static StreamingResult BorrowArenaResponse(Response& response) {
        UINVARIANT(response.GetArena(), "The response must be allocated on an arena");
        return StreamingResult(std::ref(response));
    }

    /// @returns `true` if the `StreamingResult` contains `OK` status, possibly with a last response.
    bool IsSuccess() const { return status_.ok(); }

    /// @returns the contained status, which can be `OK` or an error status.
    grpc::Status&& ExtractStatus() { return std::move(status_); }

    /// @returns the contained status, which can be `OK` or an error status.
    const grpc::Status& GetStatus() const { return status_; }

    /// @returns `true` if the `StreamingResult` contains last response, which implies `OK` status.
    bool HasLastResponse() const { return last_response_.has_value(); }

    /// @returns the contained last response.
    /// @pre `HasLastResponse() == true`.
    /// @warning Extracting an arena-backed response is usually a mistake: moving it into a standalone message
    /// copies its data off the arena. The returned reference does not extend the arena's lifetime.
    /// Prefer @ref ugrpc::server::StreamingResult::GetLastResponse for read-only access.
    Response&& ExtractLastResponse() {
        UINVARIANT(HasLastResponse(), "There is no last response in the StreamingResult");
        return std::move(last_response_->Get());
    }

    /// @returns the contained last response.
    /// @pre `HasLastResponse() == true`.
    const Response& GetLastResponse() const {
        UINVARIANT(HasLastResponse(), "There is no last response in the StreamingResult");
        return last_response_->Get();
    }

    /// @cond
    // For internal use only.
    impl::MessageStorage<Response>&& ExtractStorage(utils::impl::InternalTag) { return std::move(*last_response_); }
    /// @endcond

private:
    explicit StreamingResult(std::reference_wrapper<Response> response)
        : last_response_(std::in_place, response)
    {}

    std::optional<impl::MessageStorage<Response>> last_response_;
    grpc::Status status_{grpc::Status::OK};
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END

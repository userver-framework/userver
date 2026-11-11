#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <y_absl/functional/any_invocable.h>
#include <y_absl/status/status.h>

#include <utility>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

class PosixEngineClosure final {
public:
    explicit PosixEngineClosure(y_absl::AnyInvocable<void(y_absl::Status)> cb) noexcept : cb_(std::move(cb)) {
        UASSERT(cb_);
    }

    PosixEngineClosure(const PosixEngineClosure&) = delete;
    PosixEngineClosure& operator=(const PosixEngineClosure&) = delete;

    void SetStatus(y_absl::Status status) noexcept { status_ = std::move(status); }

    void Run() noexcept { cb_(std::exchange(status_, y_absl::OkStatus())); }

private:
    y_absl::AnyInvocable<void(y_absl::Status)> cb_;
    y_absl::Status status_;
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif

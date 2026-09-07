#pragma once

#include <memory>
#include <mutex>
#include <utility>

#include <userver/utils/assert.hpp>

#include "base_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

/// Non-template base so callers can `dynamic_cast` and reuse the mutex that
/// @ref components::Run already installed on the default logger.
class SynchronizedSinkBase : public BaseSink {
public:
    explicit SynchronizedSinkBase(std::shared_ptr<std::mutex> mutex)
        : mutex_(std::move(mutex))
    {
        UASSERT_MSG(mutex_, "SynchronizedSink requires a non-null mutex");
    }

    const std::shared_ptr<std::mutex>& GetMutex() const noexcept { return mutex_; }

protected:
    std::mutex& GetMutexRef() const noexcept { return *mutex_; }

private:
    std::shared_ptr<std::mutex> mutex_;
};

/// Decorates a sink with a shared mutex so concurrent writers to the same
/// destination do not tear log records.
///
/// Calls the underlying sink methods via `Sink::` to avoid virtual dispatch.
template <typename Sink>
class SynchronizedSink final : public SynchronizedSinkBase {
public:
    template <typename... SinkArgs>
    SynchronizedSink(std::shared_ptr<std::mutex> mutex, SinkArgs&&... args)
        : SynchronizedSinkBase(std::move(mutex)),
          sink_(std::forward<SinkArgs>(args)...)
    {}

    void Write(std::span<const IoVec> messages) override {
        const std::lock_guard lock{GetMutexRef()};
        sink_.Sink::Write(messages);
    }

    void Reopen(ReopenMode mode) override {
        const std::lock_guard lock{GetMutexRef()};
        sink_.Sink::Reopen(mode);
    }

private:
    Sink sink_;
};

template <typename Sink, typename... SinkArgs>
std::unique_ptr<BaseSink> MakeSynchronizedSink(std::shared_ptr<std::mutex> mutex, SinkArgs&&... args) {
    return std::make_unique<SynchronizedSink<Sink>>(std::move(mutex), std::forward<SinkArgs>(args)...);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END

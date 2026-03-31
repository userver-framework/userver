#pragma once

#include <exception>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

/// The engine-facing side of an asynchronous task payload. The engine will
/// call one of `Perform` or the destructor exactly once.
class WrappedCallBase {
public:
    WrappedCallBase(WrappedCallBase&&) = delete;
    virtual ~WrappedCallBase();

    /// Invoke the wrapped function call, then destroy the functor
    /// (but not the held result)
    virtual void Perform() = 0;

    /// Shows whether the result contains an exception or not.
    virtual std::exception_ptr GetException() const noexcept = 0;

protected:
    WrappedCallBase() noexcept;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END

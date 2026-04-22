#pragma once

#include <cstdint>
#include <exception>

#include <boost/smart_ptr/intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

enum class [[nodiscard]] EarlyNotify : bool { kNo = false, kYes = true };

class Awaiter;
class TaskContext;

class ContextAccessor {
public:
    virtual bool IsReady() const noexcept = 0;

    // Atomically:
    //
    // 1. If not `IsReady`, then
    //    * move from `awaiter`;
    //    * store `awaiter` and `context` somewhere to notify when `IsReady() == true` is reached.
    // 2. If `IsReady`, then
    //    * do not move from `awaiter`;
    //    * do not notify `awaiter`.
    //
    // You may not sleep in `TryAppendAwaiter`.
    virtual void TryAppendAwaiter(boost::intrusive_ptr<Awaiter>& awaiter, std::uintptr_t context) = 0;

    // Remove `awaiter` from the internal wait list if it's still there.
    // Depending on a wait list implementation `context` match also could be required for the awaiter removal.
    // You may not sleep in `RemoveAwaiter`.
    virtual void RemoveAwaiter(Awaiter& awaiter, std::uintptr_t context) noexcept = 0;

    // Returns the stored exception if present.
    // Precondition: IsReady.
    // This method is required for WaitAllChecked to properly function.
    virtual std::exception_ptr GetErrorResult() const noexcept { return {}; }

protected:
    ContextAccessor();

    ~ContextAccessor() = default;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END

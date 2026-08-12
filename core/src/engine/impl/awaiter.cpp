#include <userver/engine/impl/awaiter.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

TaskContext& Awaiter::CastToTaskContext() noexcept {
    UASSERT(type_ == StaticType::kTaskContext);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<TaskContext&>(*this);
}

Awaiter::Awaiter(StaticType type) noexcept
    : type_(type)
{}

void Awaiter::NotifyAndDisposeTaskContext(Awaiter* self, std::uintptr_t context) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    boost::intrusive_ptr<TaskContext> task_context(&static_cast<TaskContext&>(*self), /*add_ref=*/false);
    TaskContext::Wakeup(std::move(task_context), TaskContext::WakeupSource::kNotify, static_cast<Epoch>(context));
}

void Awaiter::DisposeWithoutNotificationTaskContext(Awaiter* self) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    intrusive_ptr_release(&static_cast<TaskContext&>(*self));
}

PolymorphicAwaiter::PolymorphicAwaiter()
    : Awaiter(StaticType::kPolymorphic)
{}

}  // namespace engine::impl

USERVER_NAMESPACE_END

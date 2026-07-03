#pragma once

#include <cstddef>
#include <cstdint>

#include <boost/intrusive/list_hook.hpp>

#include <userver/engine/impl/awaiter_fwd.hpp>
#include <userver/engine/impl/epoch.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class Awaiter {
public:
    Awaiter(const Awaiter&) = delete;
    Awaiter(Awaiter&&) = delete;
    Awaiter& operator=(const Awaiter&) = delete;
    Awaiter& operator=(Awaiter&&) = delete;

    TaskContext& CastToTaskContext() noexcept;

protected:
    enum class StaticType : std::uint8_t { kPolymorphic, kTaskContext };

    explicit Awaiter(StaticType type) noexcept;
    ~Awaiter() = default;

private:
    using WaitListHook = typename boost::intrusive::make_list_member_hook<
        boost::intrusive::link_mode<boost::intrusive::auto_unlink>>::type;

    struct WaitListData : public WaitListHook {
        // WaitList can't store context. But, we can store it here because an awaiter
        // can't be simultaneously linked to multiple wait lists.
        std::uintptr_t context{0};
    };

    friend struct AwaiterDeleter;
    friend class WaitList;

    friend void NotifyAndDispose(AwaiterPtr&& awaiter, std::uintptr_t context) noexcept;

    static void NotifyAndDisposeTaskContext(Awaiter* self, std::uintptr_t context) noexcept;
    static void DisposeWithoutNotificationTaskContext(Awaiter* self) noexcept;

    StaticType type_;
    WaitListData wait_list_data_;
};

struct AwaiterDeleter {
    void operator()(Awaiter* awaiter) noexcept;
};

using AwaiterPtr = std::unique_ptr<Awaiter, AwaiterDeleter>;

class PolymorphicAwaiter : public Awaiter {
public:
    virtual void NotifyAndDispose(std::uintptr_t context) noexcept = 0;

    virtual void DisposeWithoutNotification() noexcept = 0;

protected:
    PolymorphicAwaiter();
    ~PolymorphicAwaiter() = default;
};

inline void NotifyAndDispose(AwaiterPtr&& awaiter, std::uintptr_t context) noexcept {
    UASSERT(awaiter);

    switch (awaiter->type_) {
        case Awaiter::StaticType::kTaskContext:
            Awaiter::NotifyAndDisposeTaskContext(awaiter.release(), context);
            break;
        case Awaiter::StaticType::kPolymorphic:
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<PolymorphicAwaiter&>(*awaiter.release()).NotifyAndDispose(context);
            break;
    }
}

inline void AwaiterDeleter::operator()(Awaiter* awaiter) noexcept {
    UASSERT(awaiter);

    switch (awaiter->type_) {
        case Awaiter::StaticType::kTaskContext:
            Awaiter::DisposeWithoutNotificationTaskContext(awaiter);
            break;
        case Awaiter::StaticType::kPolymorphic:
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<PolymorphicAwaiter&>(*awaiter).DisposeWithoutNotification();
            break;
    }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END

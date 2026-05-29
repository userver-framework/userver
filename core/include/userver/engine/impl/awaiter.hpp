#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <boost/intrusive/list_hook.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <userver/engine/impl/epoch.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class PolymorphicAwaiter;

class Awaiter {
public:
    enum InitialRefCounter : std::size_t { kZero = 0, kOne = 1 };  // NOLINT(performance-enum-size)

    Awaiter(const Awaiter&) = delete;
    Awaiter(Awaiter&&) = delete;
    Awaiter& operator=(const Awaiter&) = delete;
    Awaiter& operator=(Awaiter&&) = delete;

    std::size_t UseCount() const noexcept;

    friend void intrusive_ptr_add_ref(Awaiter* awaiter) noexcept;  // NOLINT(readability-identifier-naming)
    friend void intrusive_ptr_release(Awaiter* awaiter) noexcept;  // NOLINT(readability-identifier-naming)

protected:
    enum StaticType : std::uint8_t { kPolymorphic, kTaskContext };

    Awaiter(StaticType type, InitialRefCounter initial_ref_counter);
    ~Awaiter() = default;

private:
    using WaitListHook = typename boost::intrusive::make_list_member_hook<
        boost::intrusive::link_mode<boost::intrusive::auto_unlink>>::type;

    struct WaitListData : public WaitListHook {
        // WaitList can't store context. But, we can store it here because an awaiter
        // can't be simultaneously linked to multiple wait lists.
        std::uintptr_t context{0};
    };

    friend class WaitList;
    friend void Notify(boost::intrusive_ptr<Awaiter>&& awaiter, std::uintptr_t context) noexcept;

    static void NotifyTaskContext(boost::intrusive_ptr<Awaiter> self, std::uintptr_t context) noexcept;

    PolymorphicAwaiter& CastToPolymorphic() noexcept;

    // refcounter for resources and memory deallocation
    std::atomic<std::size_t> intrusive_refcount_{0};

    StaticType type_;

    WaitListData wait_list_data_;
};

class PolymorphicAwaiter : public Awaiter {
public:
    virtual void DoNotify(boost::intrusive_ptr<PolymorphicAwaiter> self, std::uintptr_t context) noexcept = 0;

protected:
    PolymorphicAwaiter();
    explicit PolymorphicAwaiter(InitialRefCounter initial_ref_counter);

    ~PolymorphicAwaiter() = default;

    // Called from intrusive_ptr_release. Should delete the instance
    virtual void Destroy() noexcept = 0;

private:
    friend void intrusive_ptr_release(Awaiter* awaiter) noexcept;  // NOLINT(readability-identifier-naming)
};

inline void Notify(boost::intrusive_ptr<Awaiter>&& awaiter, std::uintptr_t context) noexcept {
    UASSERT(awaiter);

    if (awaiter->type_ == Awaiter::StaticType::kTaskContext) {
        Awaiter::NotifyTaskContext(std::move(awaiter), context);
    } else {
        auto polymorphic_awaiter = boost::static_pointer_cast<PolymorphicAwaiter>(std::move(awaiter));
        polymorphic_awaiter->DoNotify(std::move(polymorphic_awaiter), context);
    }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END

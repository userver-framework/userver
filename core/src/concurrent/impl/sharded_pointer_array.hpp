#pragma once

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <thread>
#include <utility>
#include <variant>

#ifdef __linux__
#include <sched.h>
#else
#include <functional>
#endif

#include <concurrent/impl/striped_array.hpp>
#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/forward_like.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

// CPU-sharded, zero-initialized words. The selected index stays valid after CPU migration.
class ShardedPointerArray final {
public:
    ShardedPointerArray();

    ShardedPointerArray(const ShardedPointerArray&) = delete;
    ShardedPointerArray(ShardedPointerArray&&) = delete;

    std::size_t GetCurrentShardId() const noexcept {
#ifdef __linux__
        const int cpu = ::sched_getcpu();
        if (cpu < 0) {
            return 0;
        }
        const auto shard_id = static_cast<std::size_t>(cpu);
        const auto size = GetSize();
        // The fallback array can be smaller than the range of CPU IDs allowed by the affinity mask.
        return shard_id < size ? shard_id : shard_id % size;
#else
        return std::hash<std::thread::id>{}(std::this_thread::get_id()) % GetSize();
#endif
    }

    std::intptr_t& operator[](std::size_t index) noexcept { return At(*this, index); }

    const std::intptr_t& operator[](std::size_t index) const noexcept { return At(*this, index); }

    auto Elements() {
        return Indices() | std::views::transform([this](std::size_t i) -> auto& { return (*this)[i]; });
    }

    auto Elements() const {
        return Indices() | std::views::transform([this](std::size_t i) -> const auto& { return (*this)[i]; });
    }

private:
    using FallbackArray = utils::FixedArray<InterferenceShield<std::intptr_t>>;

    static std::size_t GetFallbackSize() noexcept { return std::max(1u, std::thread::hardware_concurrency()); }

    static auto MakeStorage() {
#ifdef USERVER_IMPL_HAS_RSEQ
        if (impl::GetRseqArraySize() != 0) {
            return Storage{std::in_place_type<StripedArray>};
        } else {
            return Storage{std::in_place_type<FallbackArray>, GetFallbackSize()};
        }
#else
        return FallbackArray{GetFallbackSize()};
#endif
    }

    std::size_t GetSize() const noexcept {
#ifdef USERVER_IMPL_HAS_RSEQ
        return utils::Visit(
            storage_,
            [](const FallbackArray& fallback) { return fallback.size(); },
            [](const StripedArray&) { return impl::GetRseqArraySizeUnsafe(); }
        );
#else
        return storage_.size();
#endif
    }

    std::ranges::iota_view<std::size_t, std::size_t> Indices() const noexcept { return std::views::iota(std::size_t{0}, GetSize()); }

    template <typename Self, typename T>
    using RefLike = decltype(utils::ForwardLike<Self&>(std::declval<T&>()));

    template <typename Self>
    static RefLike<Self, std::intptr_t> At(Self& self, std::size_t index) noexcept {
        UASSERT(index < self.GetSize());
#ifdef USERVER_IMPL_HAS_RSEQ
        return utils::Visit(
            self.storage_,
            [index](RefLike<Self, FallbackArray> fallback) -> auto& { return *fallback[index]; },
            [index](RefLike<Self, StripedArray> striped) -> auto& { return striped[index]; }
        );
#else
        return *self.storage_[index];
#endif
    }

#ifdef USERVER_IMPL_HAS_RSEQ
    using Storage = std::variant<FallbackArray, StripedArray>;
    Storage storage_;
#else
    FallbackArray storage_;
#endif
};

inline ShardedPointerArray::ShardedPointerArray()
    : storage_(MakeStorage())
{}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

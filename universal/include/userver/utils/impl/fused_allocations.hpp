#pragma once

#include <cstddef>
#include <new>

#include <userver/utils/assert.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

namespace fused_allocations {

constexpr std::size_t AlignUp(std::size_t offset, std::size_t alignment) {
    UASSERT_MSG((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2");
    return (offset + alignment - 1) & ~(alignment - 1);
}

template <typename T>
constexpr std::size_t AdvanceOffset(std::size_t offset, std::size_t count) {
    offset = fused_allocations::AlignUp(offset, alignof(T));
    return offset + (sizeof(T) * count);
}

template <typename T>
std::size_t SetSpan(std::byte* base, std::size_t offset, utils::span<T>& result_span, std::size_t count) {
    UASSERT_MSG(reinterpret_cast<std::uintptr_t>(base) % alignof(T) == 0, "base must be sufficiently aligned");
    offset = fused_allocations::AlignUp(offset, alignof(T));
    result_span = utils::span<T>(reinterpret_cast<T*>(base + offset), count);
    return offset + (sizeof(T) * count);
}

template <typename Header, typename... T>
std::size_t CommonAlignment() {
    std::size_t common_alignment = alignof(Header);
    ((common_alignment = (common_alignment > alignof(T) ? common_alignment : alignof(T))), ...);
    return common_alignment;
}

}  // namespace fused_allocations

template <typename T>
struct FusedArray {
    std::size_t size;
    utils::span<T>& result_span;
};

template <typename T>
FusedArray(std::size_t, utils::span<T>&) -> FusedArray<T>;

template <typename Header, typename... T>
utils::NotNull<Header*> AllocateFused(FusedArray<T>... args) {
    std::size_t offset = fused_allocations::AlignUp(sizeof(Header), alignof(Header));

    std::size_t total_size = offset;
    ((total_size = fused_allocations::AdvanceOffset<T>(total_size, args.size)), ...);

    std::size_t common_alignment = fused_allocations::CommonAlignment<Header, T...>();
    auto* memory = reinterpret_cast<std::byte*>(::operator new(total_size, std::align_val_t{common_alignment}));

    auto* header = reinterpret_cast<Header*>(memory);

    offset = fused_allocations::AlignUp(sizeof(Header), alignof(Header));
    ((offset = fused_allocations::SetSpan(memory, offset, args.result_span, args.size)), ...);

    return utils::NotNull<Header*>(header);
}

template <typename Header, typename... T>
void DeallocateFused(utils::NotNull<Header*> header) {
    std::size_t common_alignment = fused_allocations::CommonAlignment<Header, T...>();
    ::operator delete(header.GetBase(), std::align_val_t{common_alignment});
}

}  // namespace utils::impl

USERVER_NAMESPACE_END

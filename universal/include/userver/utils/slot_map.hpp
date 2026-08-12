#pragma once

/// @file userver/utils/slot_map.hpp
/// @brief @copybrief utils::SlotMap

#include <concepts>
#include <cstddef>
#include <deque>
#include <ranges>
#include <utility>
#include <variant>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl::slot_map {

template <typename Range>
concept HasCapacity = requires(Range range) {
    {
        range.capacity()
    } -> std::integral;
};

template <typename Range>
concept Indexable = requires(Range range, std::size_t index) {
    {
        range[index]
    } -> std::convertible_to<std::ranges::range_reference_t<Range&>>;
};

inline constexpr std::size_t kFreeListEnd = std::numeric_limits<std::size_t>::max();

struct FreeNode final {
    constexpr explicit FreeNode(std::size_t next_index) noexcept
        : next_index(next_index)
    {}

    std::size_t next_index{kFreeListEnd};
};

}  // namespace impl::slot_map

/// @ingroup userver_containers
///
/// @brief A minimalistic slot-map container adaptor.
///
/// Each slot holds either a live value of type `T` or a free-list node.
/// Erased slots are recycled so that `emplace` reuses them before allocating new storage.
///
/// Guarantees:
///
/// - `operator[](std::size_t)` is O(1);
/// - `emplace` and `insert` are amortized O(1) (assuming that `Container` provides this guarantee);
/// - `erase` is O(1);
/// - `size()` and `empty()` are O(1);
/// - iterators returned by `AliveItems()` ARE invalidated by `emplace`, `insert`,
///   `insert_range`, and `erase`, regardless of the underlying `Container`.
///
/// If `Container` is `std::deque` or another reference-stable container, then inserted elements have reference
/// stability; otherwise elements may move around on `emplace` and `insert*`, and `T` should be movable.
///
/// `Container` template is instantiated with an internal value type; the resulting type must:
///
/// - be a `std::ranges::forward_range` and `std::ranges::sized_range`;
/// - support `operator[](std::size_t)` with computational complexity O(1);
/// - support `emplace_back`, forwarding arbitrary `Args&&...` to the value;
/// - support `std::ranges::size` with computational complexity O(1).
///
/// For example, `std::deque`, `std::vector`, `boost::small_vector` satisfy those requirements.
///
/// @note Not thread-safe.
template <typename T, template <typename...> typename Container = std::deque>
class SlotMap final {
    using Slot = std::variant<T, impl::slot_map::FreeNode>;

    static_assert(std::ranges::forward_range<Container<Slot>&>);
    static_assert(std::ranges::forward_range<const Container<Slot>&>);
    static_assert(std::ranges::sized_range<const Container<Slot>&>);
    static_assert(impl::slot_map::Indexable<Container<Slot>&>);
    static_assert(impl::slot_map::Indexable<const Container<Slot>&>);

public:
    /// @brief Result of an `emplace` or `insert` call.
    struct InsertionResult {
        T& element;         ///< Reference to the newly inserted element.
        std::size_t index;  ///< Stable index of the newly inserted element.
    };

    /// @brief Constructs an empty SlotMap.
    SlotMap() = default;

    /// @brief Constructs a SlotMap from the elements in `[first, last)`.
    template <std::input_iterator It, std::sentinel_for<It> Sentinel>
    SlotMap(It first, Sentinel last) {
        insert_range(std::ranges::subrange(std::move(first), std::move(last)));
    }

    SlotMap(const SlotMap&) = default;
    SlotMap& operator=(const SlotMap&) = default;

    SlotMap(SlotMap&&) noexcept = default;
    SlotMap& operator=(SlotMap&&) noexcept = default;

    ~SlotMap() = default;

    /// @brief Constructs a new element in-place and returns a reference to it and its stable index.
    ///
    /// If there are free (erased) slots, one is reused; otherwise new storage is allocated.
    ///
    /// @note Invalidates all iterators returned by `AliveItems()`.
    /// Does not invalidate references to existing elements if the underlying container supports it, e.g. `std::deque`.
    template <typename... Args>
    InsertionResult emplace(Args&&... args) USERVER_IMPL_LIFETIME_BOUND {
        if (const auto entry = TryPopFromFreeList(); entry.slot != nullptr) {
            try {
                T& element = entry.slot->template emplace<T>(std::forward<Args>(args)...);
                return {element, entry.index};
            } catch (...) {
                EraseAndPushToFreeList(entry);
                throw;
            }
        }

        const std::size_t index = std::ranges::size(slots_);
        Slot& slot = slots_.emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
        auto* const value = std::get_if<T>(&slot);
        UASSERT(value != nullptr);
        return {*value, index};
    }

    /// @brief Inserts a copy of @a value and returns a reference to it and its stable index.
    ///
    /// @note Invalidates all iterators returned by `AliveItems()`.
    /// Does not invalidate references to existing elements if the underlying container supports it, e.g. `std::deque`.
    InsertionResult insert(const T& value) USERVER_IMPL_LIFETIME_BOUND { return emplace(value); }

    /// @brief Inserts @a value by move and returns a reference to it and its stable index.
    ///
    /// @note Invalidates all iterators returned by `AliveItems()`.
    /// Does not invalidate references to existing elements if the underlying container supports it, e.g. `std::deque`.
    InsertionResult insert(T&& value) USERVER_IMPL_LIFETIME_BOUND { return emplace(std::move(value)); }

    /// @brief Inserts all elements from @a range.
    ///
    /// Provides a basic exception guarantee: if an element's constructor throws,
    /// all previously inserted elements remain valid and the map is left in a
    /// consistent state.
    ///
    /// @note Invalidates all iterators returned by `AliveItems()`.
    /// Does not invalidate references to existing elements if the underlying container supports it, e.g. `std::deque`.
    template <std::ranges::input_range Range>
    void insert_range(Range&& range) {
        for (auto&& elem : std::forward<Range>(range)) {
            emplace(std::forward<decltype(elem)>(elem));
        }
    }

    /// @brief Returns the number of live elements.
    [[nodiscard]] std::size_t size() const noexcept { return std::ranges::size(slots_) - free_list_size_; }

    /// @brief Returns true if there are no live elements.
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    /// @brief Returns the total number of allocated slots (live + free).
    ///
    /// This is the size of the backing storage. It never shrinks.
    [[nodiscard]] std::size_t capacity() const noexcept {
        if constexpr (impl::slot_map::HasCapacity<const Container<Slot>&>) {
            return slots_.capacity();
        } else {
            return std::ranges::size(slots_);
        }
    }

    /// @brief Calls `reserve` on the underlying container.
    void reserve(std::size_t capacity)
    requires meta::IsReservable<Container<Slot>>
    {
        slots_.reserve(capacity);
    }

    /// @brief Returns a reference to the live element at @a index.
    ///
    /// Precondition: the slot at @a index holds a live element (was not erased).
    T& operator[](std::size_t index) noexcept USERVER_IMPL_LIFETIME_BOUND {
        UASSERT(index < std::ranges::size(slots_));
        auto* const value = std::get_if<T>(&slots_[index]);
        UASSERT(value != nullptr);
        return *value;
    }

    /// @overload
    const T& operator[](std::size_t index) const noexcept USERVER_IMPL_LIFETIME_BOUND {
        UASSERT(index < std::ranges::size(slots_));
        const auto* const value = std::get_if<T>(&slots_[index]);
        UASSERT(value != nullptr);
        return *value;
    }

    /// @brief Destroys the element at @a index and recycles the slot.
    ///
    /// If the slot at @a index is already free (was previously erased), this is a no-op.
    ///
    /// @returns the number of erased elements: `1` if a live element was erased,
    /// `0` if the slot was already free.
    ///
    /// @note Invalidates all iterators returned by `AliveItems()`.
    /// Does NOT invalidate references to other live elements.
    std::size_t erase(std::size_t index) {
        UASSERT(index < std::ranges::size(slots_));
        auto& slot = slots_[index];
        if (std::get_if<T>(&slot) == nullptr) {
            return 0;
        }
        EraseAndPushToFreeList({.slot = &slot, .index = index});
        return 1;
    }

    /// @brief Returns a view over live (non-erased) elements, yielding `T&` references.
    ///
    /// @warning Iterators of the returned view are invalidated by `emplace`, `insert`,
    /// `insert_range`, and `erase`. References to elements remain valid.
    [[nodiscard]] std::ranges::forward_range auto AliveItems() noexcept USERVER_IMPL_LIFETIME_BOUND {
        return std::ranges::ref_view(slots_) | std::views::filter(IsLive{}) | std::views::transform(ToValue{});
    }

    /// @overload
    [[nodiscard]] std::ranges::forward_range auto AliveItems() const noexcept USERVER_IMPL_LIFETIME_BOUND {
        return std::ranges::ref_view(slots_) | std::views::filter(IsLive{}) | std::views::transform(ToConstValue{});
    }

private:
    struct IsLive {
        bool operator()(const Slot& slot) const noexcept { return std::holds_alternative<T>(slot); }
    };

    struct ToValue {
        T& operator()(Slot& slot) const noexcept {
            auto* const value = std::get_if<T>(&slot);
            UASSERT(value != nullptr);
            return *value;
        }
    };

    struct ToConstValue {
        const T& operator()(const Slot& slot) const noexcept {
            const auto* const value = std::get_if<T>(&slot);
            UASSERT(value != nullptr);
            return *value;
        }
    };

    struct FreeListEntry final {
        Slot* slot;
        std::size_t index;
    };

    FreeListEntry TryPopFromFreeList() noexcept {
        const auto index = free_list_head_;

        if (index == impl::slot_map::kFreeListEnd) {
            return {.slot = nullptr, .index = impl::slot_map::kFreeListEnd};
        }

        Slot& slot = slots_[index];
        const auto* const node = std::get_if<impl::slot_map::FreeNode>(&slot);
        UASSERT(node != nullptr);
        free_list_head_ = node->next_index;
        --free_list_size_;
        return {.slot = &slot, .index = index};
    }

    void EraseAndPushToFreeList(FreeListEntry entry) noexcept {
        UASSERT(entry.slot != nullptr);
        UASSERT(entry.slot == &slots_[entry.index]);
        entry.slot->template emplace<impl::slot_map::FreeNode>(free_list_head_);
        free_list_head_ = entry.index;
        ++free_list_size_;
    }

    Container<Slot> slots_;
    std::size_t free_list_head_{impl::slot_map::kFreeListEnd};
    std::size_t free_list_size_{0};
};

}  // namespace utils

USERVER_NAMESPACE_END

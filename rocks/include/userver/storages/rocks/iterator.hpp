#pragma once

/// @file userver/storages/rocks/iterator.hpp
/// @brief @copybrief storages::rocks::Iterator

#include <cstdint>
#include <string_view>
#include <userver/storages/rocks/detail/iterator_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

enum class IteratorDirection : std::int8_t { kForward, kBackward };

class KeyValueView final {
public:
    [[nodiscard]] auto Key() const { return iterator_impl_->Key(); }
    [[nodiscard]] auto Value() const { return iterator_impl_->Value(); }

private:
    template <IteratorDirection>
    friend class Iterator;

    KeyValueView(const detail::IteratorImpl* iterator) noexcept : iterator_impl_{iterator} {}
    void Reset(const detail::IteratorImpl* iterator) noexcept { iterator_impl_ = iterator; }

    const detail::IteratorImpl* iterator_impl_;
};

template <IteratorDirection D>
class IteratorSentinel {};

template <IteratorDirection D = IteratorDirection::kForward>
class Iterator final {
public:
    using value_type = KeyValueView;
    using reference = value_type&;
    using pointer = value_type*;

    Iterator(detail::IteratorImpl&& iterator) noexcept
        : iterator_impl_{std::move(iterator)}, kv_view_{&iterator_impl_} {}

    Iterator(Iterator&& other) noexcept
        : iterator_impl_{std::move(other.iterator_impl_)}, kv_view_{&iterator_impl_} {}

    Iterator& operator=(Iterator&& other) noexcept {
        iterator_impl_ = std::move(other.iterator_impl_);
        kv_view_.Reset(&iterator_impl_);
        return *this;
    }

    [[nodiscard]] bool Valid() const noexcept {
        return iterator_impl_.Valid();
    }

    bool operator==(const IteratorSentinel<D>&) const {
        return !iterator_impl_.Valid();
    }

    bool operator!=(const IteratorSentinel<D>& sentinel) const {
        return !(*this == sentinel);
    }

    reference operator*() const {
        return kv_view_;
    }

    pointer operator->() const {
        return &kv_view_;
    }

    Iterator& operator++() {
        if constexpr (D == IteratorDirection::kForward) {
            iterator_impl_.Next();
        } else {
            iterator_impl_.Prev();
        }
        return *this;
    }

    Iterator& operator--() {
        if constexpr (D == IteratorDirection::kForward) {
            iterator_impl_.Prev();
        } else {
            iterator_impl_.Next();
        }
        return *this;
    }

    void SeekToFirst() & {
        if constexpr (D == IteratorDirection::kForward) {
            iterator_impl_.SeekToFirst();
        } else {
            iterator_impl_.SeekToLast();
        }
    }

    [[nodiscard]] auto SeekToFirst() && {
        SeekToFirst();
        return std::move(*this);
    }

    void SeekToLast() & {
        if constexpr (D == IteratorDirection::kForward) {
            iterator_impl_.SeekToLast();
        } else {
            iterator_impl_.SeekToFirst();
        }
    }

    [[nodiscard]] auto SeekToLast() && {
        SeekToLast();
        return std::move(*this);
    }

    void Seek(std::string_view key) & {
        if constexpr (D == IteratorDirection::kForward) {
            iterator_impl_.Seek(key);
        } else {
            iterator_impl_.SeekForPrev(key);
        }
    }

    [[nodiscard]] auto Seek(std::string_view key) && {
        Seek(key);
        return std::move(*this);
    }

    void SeekForPrev(std::string_view key) & {
        if constexpr (D == IteratorDirection::kForward) {
            iterator_impl_.SeekForPrev(key);
        } else {
            iterator_impl_.Seek(key);
        }
    }

    [[nodiscard]] auto SeekForPrev(std::string_view key) && {
        SeekForPrev(key);
        return std::move(*this);
    }

private:
    detail::IteratorImpl iterator_impl_;
    mutable value_type kv_view_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END

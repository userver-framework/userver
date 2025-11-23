#pragma once

/// @file userver/storages/rocks/snapshot.hpp
/// @brief @copybrief storages::rocks::Snapshot

#include <string>
#include <cstdint>
#include <optional>
#include <string_view>
#include <userver/storages/rocks/iterator.hpp>
#include <userver/storages/rocks/column_family.hpp>
#include <userver/storages/rocks/detail/snapshot_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

enum class SnapshotRangeLayout : std::uint8_t { kDefault = 0, kColumnFamily };

namespace detail {

template <SnapshotRangeLayout L>
class SnapshotRangeImpl;

template <class T>
class SnapshotRangeBase {
public:
    template <IteratorDirection D>
    auto NewIterator() const {
        return static_cast<const T*>(this)->template NewIterator<D>();
    }

private:
    friend T;
    SnapshotRangeBase(const detail::SnapshotImpl& snapshot) noexcept : snapshot_impl_{snapshot} {}
    detail::SnapshotImpl snapshot_impl_;
};

template <>
class SnapshotRangeImpl<SnapshotRangeLayout::kDefault>
    : public SnapshotRangeBase<SnapshotRangeImpl<SnapshotRangeLayout::kDefault>> {
public:
    SnapshotRangeImpl(const detail::SnapshotImpl& snapshot) noexcept : SnapshotRangeBase{snapshot} {}

    template <IteratorDirection D>
    Iterator<D> NewIterator() const;
};

template <>
class SnapshotRangeImpl<SnapshotRangeLayout::kColumnFamily>
    : public SnapshotRangeBase<SnapshotRangeImpl<SnapshotRangeLayout::kColumnFamily>> {
public:
    SnapshotRangeImpl(const detail::SnapshotImpl& snapshot, ColumnFamilyHandle column_family) noexcept
        : SnapshotRangeBase{snapshot}, column_family_{column_family} {}

    template <IteratorDirection D>
    Iterator<D> NewIterator() const;

private:
    ColumnFamilyHandle column_family_;
};

}  // namespace detail

class Db;

template <SnapshotRangeLayout L>
class SnapshotRange final : public detail::SnapshotRangeImpl<L> {
public:
    using detail::SnapshotRangeImpl<L>::SnapshotRangeImpl;

    [[nodiscard]] auto begin() const {
        return this->template NewIterator<IteratorDirection::kForward>().SeekToFirst();
    }

    [[nodiscard]] const Iterator<IteratorDirection::kForward> cbegin() const {
        return begin();
    }

    [[nodiscard]] auto end() const {
        return IteratorSentinel<IteratorDirection::kForward>{};
    }

    [[nodiscard]] const IteratorSentinel<IteratorDirection::kForward> cend() const {
        return end();
    }

    [[nodiscard]] auto rbegin() const {
        return this->template NewIterator<IteratorDirection::kBackward>().SeekToFirst();
    }

    [[nodiscard]] const Iterator<IteratorDirection::kBackward> crbegin() const {
        return rbegin();
    }

    [[nodiscard]] auto rend() const {
        return IteratorSentinel<IteratorDirection::kBackward>{};
    }

    [[nodiscard]] const IteratorSentinel<IteratorDirection::kBackward> crend() const {
        return rend();
    }
};

class Snapshot final {
public:
    Snapshot(detail::SnapshotImpl&& snapshot) noexcept : snapshot_impl_{std::move(snapshot)} {}

    [[nodiscard]] auto AsRange() const {
        return SnapshotRange<SnapshotRangeLayout::kDefault>{snapshot_impl_};
    }

    [[nodiscard]] auto AsRange(ColumnFamilyHandle column_family) const {
        return SnapshotRange<SnapshotRangeLayout::kColumnFamily>{snapshot_impl_, column_family};
    }

    [[nodiscard]] std::optional<std::string> Get(std::string_view key) const;
    [[nodiscard]] std::optional<std::string> Get(ColumnFamilyHandle column_family, std::string_view key) const;

    template <IteratorDirection Direction = IteratorDirection::kForward>
    [[nodiscard]] Iterator<Direction> NewIterator() const;

    template <IteratorDirection Direction = IteratorDirection::kForward>
    [[nodiscard]] Iterator<Direction> NewIterator(ColumnFamilyHandle column_family) const;

private:
    detail::SnapshotImpl snapshot_impl_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END

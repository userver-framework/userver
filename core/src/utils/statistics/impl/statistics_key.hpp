#pragma once

/// @file userver/utils/statistics/impl/statistics_key.hpp
/// @brief Key types for statistics storage with transparent hashing.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/functional/hash.hpp>

#include <userver/utils/statistics/labels.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

struct StatisticsKeyView;

/// @brief Owning key for statistics map storage.
struct StatisticsKey final {
    StatisticsKey(std::string path, std::vector<utils::statistics::Label> labels);
    StatisticsKey() = default;

    std::string path;
    std::vector<utils::statistics::Label> labels;
    std::vector<utils::statistics::LabelView> label_views;
};

/// @brief Non-owning key view for transparent lookup (no copies).
struct StatisticsKeyView final {
    std::string_view path;
    utils::statistics::LabelsSpan labels;

    /// @brief Convert to owning key for insertion.
    StatisticsKey Materialize() const;
};

/// @brief Transparent hash - accepts both StatisticsKey and StatisticsKeyView by const reference.
/// Does not copy its arguments.
struct StatisticsKeyHash {
    using is_transparent [[maybe_unused]] = void;

    std::size_t operator()(const StatisticsKey& key) const noexcept;
    std::size_t operator()(const StatisticsKeyView& key) const noexcept;
};

/// @brief Transparent equality - compares StatisticsKey with StatisticsKeyView.
struct StatisticsKeyEqual {
    using is_transparent [[maybe_unused]] = void;

    bool operator()(const StatisticsKey& a, const StatisticsKey& b) const noexcept;
    bool operator()(const StatisticsKey& a, const StatisticsKeyView& b) const noexcept;
    bool operator()(const StatisticsKeyView& a, const StatisticsKey& b) const noexcept;
};

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END

#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace kafka {

/// @brief Represents the range of offsets for certain topic.
struct OffsetRange final {
    /// @brief The low watermark offset. It indicates the earliest available offset in Kafka.
    /// @note low offset is guaranteed to be committed. Max value is std::int64_t::max() according to Kafka protocol
    /// document.
    std::uint64_t low{};

    /// @brief The high watermark offset. It indicates the next offset that will be written in Kafka.
    /// @note high offset is not required to be committed yet. Max value is std::int64_t::max() according to Kafka
    /// protocol document.
    std::uint64_t high{};
};

}  // namespace kafka

USERVER_NAMESPACE_END

#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace kafka {

/// @brief Represents the range of offsets for certain topic.
struct OffsetRange final {
    /// @brief The low watermark offset. It indicates the earliest available offset in Kafka.
    /// @note low offset is guaranteed to be commited
    std::uint32_t low{};

    /// @brief The high watermark offset. It indicates the next offset that will be written in Kafka.
    /// @note high offset is not required to be commited yet
    std::uint32_t high{};
};

}  // namespace kafka

USERVER_NAMESPACE_END

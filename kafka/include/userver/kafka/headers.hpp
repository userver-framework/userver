#pragma once

#include <cstddef>
#include <iterator>
#include <string_view>

#include <userver/utils/span.hpp>
#include <userver/utils/zstring_view.hpp>

struct rd_kafka_headers_s;

USERVER_NAMESPACE_BEGIN

namespace kafka {

/// @brief Wrapper for Kafka header.
/// Header `name` must be valid null-terminated string.
/// Header `value` is treated as a binary data by Kafka.
struct HeaderView final {
    utils::zstring_view name;
    std::string_view value;

    bool operator==(const HeaderView&) const;
};

/// @brief Owning wrapper for Kafka header.
class OwningHeader final {
public:
    explicit OwningHeader(HeaderView header);

    OwningHeader(std::string name, std::string value);

    const std::string& GetName() const noexcept;

    std::string_view GetValue() const noexcept;

private:
    std::string name_;
    std::string value_;
};

bool operator==(const HeaderView& lhs, const OwningHeader& rhs);
bool operator==(const OwningHeader& lhs, const HeaderView& rhs);

/// @brief Ordered list of HeaderView.
/// Duplicated keys are supported.
/// Keys order is preserved.
using HeaderViews = utils::span<const HeaderView>;

/// @brief Header views iterator.
class HeadersIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = HeaderView;
    using reference = HeaderView;
    using pointer = reference*;

    HeadersIterator() = default;
    HeadersIterator(const rd_kafka_headers_s* headers, std::size_t index);

    HeadersIterator operator++(int);
    HeadersIterator& operator++();

    reference operator*() const;

    bool operator==(const HeadersIterator&) const;
    bool operator!=(const HeadersIterator&) const;

private:
    const rd_kafka_headers_s* headers_{nullptr};
    std::size_t index_{0};
};

#if defined(__cpp_concepts)
static_assert(std::forward_iterator<HeadersIterator>);
#endif

/// @brief Class to read all message's headers.
class HeadersReader {
public:
    /// @note if `name` passed, only headers with matching name are read.
    HeadersReader(const rd_kafka_headers_s*);

    HeadersIterator begin() const;
    HeadersIterator end() const;

    std::size_t size() const noexcept;

private:
    const rd_kafka_headers_s* headers_{nullptr};
    std::size_t size_;
};

}  // namespace kafka

USERVER_NAMESPACE_END

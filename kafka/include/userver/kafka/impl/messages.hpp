#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

/// @brief Message collection adapter.
class Messages {
public:
    virtual std::size_t Size() const noexcept = 0;
    virtual std::string_view operator[](std::size_t index) const noexcept = 0;

protected:
    ~Messages() = default;
};

template <typename Container>
class MessagesAdapter final : public Messages {
public:
    static_assert(
        std::is_convertible_v<decltype(std::declval<const Container&>()[0]), std::string_view>,
        "Container must support operator[] and return a type convertible to std::string_view"
    );

    static_assert(
        std::is_integral_v<decltype(std::declval<const Container&>().size())>,
        "Container must support method size() and return an integral type"
    );

    explicit MessagesAdapter(const Container& data) noexcept : data_{data} {}

    std::size_t Size() const noexcept override { return data_.size(); }
    std::string_view operator[](std::size_t index) const noexcept override { return data_[index]; }

private:
    const Container& data_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END

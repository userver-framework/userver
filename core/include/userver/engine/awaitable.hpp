#pragma once

/// @file userver/engine/awaitable.hpp
/// @brief Utilities for integration with @ref engine::WaitAny and friends.

#include <concepts>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class AwaitableBase;
}

namespace engine {

/// @brief A lightweight token that exposes a view over an awaitable to @ref engine::WaitAny and friends.
///
/// User-facing awaitable types return this token from their `GetAwaitableToken` methods.
/// Empty token means that the object cannot be awaited right now.
class AwaitableToken final {
public:
    /// @brief Creates an empty token. It will be skipped in @ref engine::WaitAny and friends.
    constexpr AwaitableToken() noexcept = default;

    AwaitableToken(const AwaitableToken&) noexcept = default;
    AwaitableToken(AwaitableToken&&) noexcept = default;
    AwaitableToken& operator=(const AwaitableToken&) noexcept = default;
    AwaitableToken& operator=(AwaitableToken&&) noexcept = default;
    ~AwaitableToken() = default;

    /// @brief Returns true if the token does not refer to an awaitable.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept { return awaitable_ == nullptr; }

    /// @brief Compares referred awaitables.
    [[nodiscard]] constexpr bool operator==(const AwaitableToken& other) const noexcept {
        return awaitable_ == other.awaitable_;
    }

    /// @cond
    // For internal use only.
    constexpr AwaitableToken(utils::impl::InternalTag, impl::AwaitableBase* awaitable) noexcept : awaitable_(awaitable) {}

    // For internal use only. Precondition: !IsEmpty().
    [[nodiscard]] impl::AwaitableBase& GetAwaitable(utils::impl::InternalTag) const noexcept {
        UASSERT(!IsEmpty());
        return *awaitable_;
    }
    /// @endcond

private:
    impl::AwaitableBase* awaitable_{nullptr};
};

/// @brief Returns a non-empty token that is always ready. Useful for mocking awaitables in tests.
AwaitableToken MakeReadyAwaitableToken() noexcept;

/// @brief A type that can expose an awaitable token for @ref engine::WaitAny and friends.
template <typename T>
concept Awaitable = requires(T& value) {
    {
        value.GetAwaitableToken()
    } -> std::same_as<AwaitableToken>;
};

}  // namespace engine

USERVER_NAMESPACE_END

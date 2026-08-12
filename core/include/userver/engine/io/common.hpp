#pragma once

/// @file userver/engine/io/common.hpp
/// @brief Common definitions and base classes for stream like objects

#include <cstddef>
#include <memory>
#include <optional>
#include <span>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class AwaitableBase;
}

namespace engine::io {

/// File descriptor of an invalid pipe end.
inline constexpr int kInvalidFd = -1;

/// @ingroup userver_base_classes
///
/// Base class for readable stream waiting
class ReadAwaiter {
public:
    virtual ~ReadAwaiter();

    /// Suspends current task until the stream has data available.
    [[nodiscard]] virtual bool WaitReadable(Deadline) = 0;

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    AwaitableToken GetAwaitableToken() USERVER_IMPL_LIFETIME_BOUND {
        return AwaitableToken{utils::impl::InternalTag{}, ca_};
    }

protected:
    void SetReadableAwaitableToken(AwaitableToken token) {
        ca_ = token.IsEmpty() ? nullptr : &token.GetAwaitable(utils::impl::InternalTag{});
    }

private:
    impl::AwaitableBase* ca_{nullptr};
};

/// @ingroup userver_base_classes
///
/// Interface for readable streams
class ReadableBase : public ReadAwaiter {
public:
    ~ReadableBase() override;

    /// Whether the stream is valid.
    virtual bool IsValid() const = 0;

    /// Receives up to len (including zero) bytes from the stream.
    /// @returns filled-in optional on data presence (e.g. 0, 1, 2... bytes)
    ///  empty optional otherwise
    [[nodiscard]] virtual std::optional<size_t> ReadNoblock(void* buf, size_t len) {
        (void)buf;
        (void)len;
        UINVARIANT(false, "not implemented yet");
        return {};
    }

    /// Receives at least one byte from the stream.
    [[nodiscard]] virtual size_t ReadSome(void* buf, size_t len, Deadline deadline) = 0;

    /// Receives exactly len bytes from the stream.
    /// @note Can return less than len if stream is closed by peer.
    [[nodiscard]] virtual size_t ReadAll(void* buf, size_t len, Deadline deadline) = 0;
};

/// @ingroup userver_base_classes
///
/// Base class for writable stream waiting
class WriteAwaiter {
public:
    virtual ~WriteAwaiter();

    /// Suspends current task until the data is available.
    [[nodiscard]] virtual bool WaitWriteable(Deadline deadline) = 0;

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    AwaitableToken GetAwaitableToken() USERVER_IMPL_LIFETIME_BOUND {
        return AwaitableToken{utils::impl::InternalTag{}, ca_};
    }

protected:
    void SetWritableAwaitableToken(AwaitableToken token) {
        ca_ = token.IsEmpty() ? nullptr : &token.GetAwaitable(utils::impl::InternalTag{});
    }

private:
    impl::AwaitableBase* ca_{nullptr};
};

/// IoData for vector send
struct IoData final {
    const void* data;
    size_t len;
};

/// @ingroup userver_base_classes
///
/// Interface for writable streams
class WritableBase : public WriteAwaiter {
public:
    ~WritableBase() override;

    /// @brief Sends exactly len bytes of buf.
    /// @note Can return less than len if stream is closed by peer.
    [[nodiscard]] virtual size_t WriteAll(const void* buf, size_t len, Deadline deadline) = 0;

    /// @brief Sends IoData array using vector I/O (e.g. writev).
    /// @note Can return less than total bytes if stream is closed by peer.
    [[nodiscard]] virtual size_t WriteAll(std::span<const IoData> list, Deadline deadline) {
        size_t result{0};
        for (const auto& io_data : list) {
            result += WriteAll(io_data.data, io_data.len, deadline);
        }
        return result;
    }

    /// @brief Sends IoData initializer list using vector I/O (e.g. writev).
    /// @note Can return less than total bytes if stream is closed by peer.
    [[nodiscard]] virtual size_t WriteAll(std::initializer_list<IoData> list, Deadline deadline) {
        return WriteAll(std::span<const IoData>{list.begin(), list.size()}, deadline);
    }
};

/// @ingroup userver_base_classes
///
/// Interface for readable and writable streams
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class RwBase : public ReadableBase, public WritableBase {
public:
    ~RwBase() override;

    ReadableBase& GetReadableBase() { return *this; }

    WritableBase& GetWritableBase() { return *this; }
};

using ReadableBasePtr = std::shared_ptr<ReadableBase>;

}  // namespace engine::io

USERVER_NAMESPACE_END

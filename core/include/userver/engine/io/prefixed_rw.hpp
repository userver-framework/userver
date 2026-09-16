#pragma once

/// @file userver/engine/io/prefixed_rw.hpp
/// @brief @copybrief engine::io::PrefixedRw

#include <memory>
#include <optional>
#include <string>

#include <userver/engine/io/common.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// @ingroup userver_base_classes
///
/// @brief @ref engine::io::RwBase adapter that returns a fixed byte prefix before reading
/// from the underlying stream.
///
/// Useful when some already-received bytes must be replayed as if they were
/// still unread on the socket (for example, handshake leftovers buffered by
/// a third-party library).
///
/// Writes always go to the underlying stream. The readable awaitable reports
/// ready while unread prefix bytes remain (so @ref engine::WaitAny wakes even
/// if the underlying socket has no data yet); afterwards it follows the
/// underlying stream.
///
/// @snippet core/src/engine/io/prefixed_rw_test.cpp  Sample PrefixedRw
class PrefixedRw final : public RwBase {
public:
    /// @param prefix Bytes returned by read APIs before touching @a underlying.
    /// @param underlying Stream used after @a prefix is exhausted; must be non-null.
    PrefixedRw(std::string prefix, std::unique_ptr<RwBase> underlying);

    PrefixedRw(const PrefixedRw&) = delete;
    PrefixedRw& operator=(const PrefixedRw&) = delete;

    PrefixedRw(PrefixedRw&&) noexcept = delete;
    PrefixedRw& operator=(PrefixedRw&&) noexcept = delete;

    ~PrefixedRw() override;

    /// Whether the underlying stream is valid.
    bool IsValid() const override;

    /// @returns true immediately while unread prefix bytes remain; otherwise
    /// waits on the underlying stream.
    [[nodiscard]] bool WaitReadable(Deadline deadline) override;

    /// Waits on the underlying stream.
    [[nodiscard]] bool WaitWriteable(Deadline deadline) override;

    /// Reads from the prefix first, then from the underlying stream.
    [[nodiscard]] std::optional<size_t> ReadNoblock(void* buf, size_t len) override;

    /// Reads from the prefix first, then from the underlying stream.
    [[nodiscard]] size_t ReadSome(void* buf, size_t len, Deadline deadline) override;

    /// Reads from the prefix first, then from the underlying stream.
    [[nodiscard]] size_t ReadAll(void* buf, size_t len, Deadline deadline) override;

    /// Forwards to the underlying stream.
    [[nodiscard]] size_t WriteAll(const void* buf, size_t len, Deadline deadline) override;

    /// Forwards to the underlying stream.
    [[nodiscard]] size_t WriteAll(std::span<const IoData> list, Deadline deadline) override;

private:
    class ReadAwaitable;

    [[nodiscard]] bool HasUnreadPrefix() const noexcept { return prefix_pos_ < prefix_.size(); }

    std::optional<size_t> ReadFromPrefix(void* buf, size_t len);

    std::string prefix_;
    std::size_t prefix_pos_{0};
    std::unique_ptr<RwBase> underlying_;
    // Size/alignment match PrefixedRw::ReadAwaitable (vptr + PrefixedRw&).
    utils::FastPimpl<ReadAwaitable, 16, 8> read_awaitable_;
};

}  // namespace engine::io

USERVER_NAMESPACE_END

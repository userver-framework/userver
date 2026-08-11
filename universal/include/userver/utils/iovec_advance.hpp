#pragma once

/// @file userver/utils/iovec_advance.hpp
/// @brief Helpers for advancing `iovec` buffers used in scatter/gather I/O
/// (`readv`/`writev`).

#include <sys/uio.h>
#include <cstddef>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal
///
/// @brief Trims the first `n` bytes off a single `iovec`: moves `iov_base`
/// forward and decreases `iov_len` by `n`.
///
/// Typically called after a partial transfer to adjust the buffer for the
/// next I/O call.
///
/// @param iov the buffer to trim.
/// @param n   number of bytes already transferred.
///
/// @warning `n` must be strictly less than `iov.iov_len`, so that the resulting
/// buffer is never empty. The precondition is only validated by `UASSERT`, i.e.
/// in debug builds; violating it in a release build yields a buffer pointing
/// outside of the original memory region.
inline void Advance(struct iovec& iov, std::size_t n) {
    UASSERT(n < iov.iov_len);
    iov.iov_base = static_cast<char*>(iov.iov_base) + n;
    iov.iov_len -= n;
}

/// @ingroup userver_universal
///
/// @brief A cursor over an array of `iovec` entries, used to track progress
/// across multiple scatter/gather buffers.
struct IovIter {
    /// Points to the first entry that has not been transferred completely.
    /// May point past the end of the array when `iov_size == 0`, so it must not
    /// be dereferenced without checking `iov_size` first.
    const struct iovec* iov{};

    /// Number of `iovec` entries remaining, starting at `iov`.
    std::size_t iov_size{};

    /// Byte offset within `*iov`, i.e. the number of bytes of the current entry
    /// that were already transferred. `0` means the current entry has not been
    /// partially consumed and the list can be reused as is.
    std::size_t iov_offset{0};
};

/// @ingroup userver_universal
///
/// @brief Advances the iterator by `n` transferred bytes across the `iovec`
/// array.
///
/// After the call:
/// - `iov_size == 0` means that everything was transferred; `iov` points past
///   the end of the array and must not be dereferenced;
/// - `iov_offset == 0` with a non-zero `iov_size` means the iterator points
///   exactly at the start of the next unconsumed entry, so `{iov, iov_size}`
///   can be passed to the next I/O call as is;
/// - a non-zero `iov_offset` means that `*iov` is partially consumed and has to
///   be trimmed by `iov_offset` bytes (e.g. via
///   utils::Advance(struct iovec&, std::size_t)) before reuse. The entries
///   themselves are never modified by this overload, as they are pointed to by
///   a pointer-to-const.
inline void Advance(IovIter& iter, std::size_t n) {
    while (0 < iter.iov_size) {
        const std::size_t iov_len = iter.iov->iov_len;
        if (iov_len <= n) {
            ++iter.iov;
            --iter.iov_size;
            n -= iov_len;
            UASSERT(0 < iter.iov_size || 0 == n);
        } else [[unlikely]] {
            iter.iov_offset = n;
            return;
        }
    }
}

}  // namespace utils

USERVER_NAMESPACE_END

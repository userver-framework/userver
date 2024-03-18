#pragma once

#include <cstddef>
#include <cstdint>
#include <thread>

#include <userver/utils/assert.hpp>

#if defined(__x86_64__) && defined(__linux__) && \
    !defined(USERVER_DISABLE_RSEQ_ACCELERATION)
#define USERVER_IMPL_HAS_RSEQ
#endif

#ifdef USERVER_IMPL_HAS_RSEQ

#include <rseq/rseq.h>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

inline std::size_t GetRseqArraySize() noexcept {
  static const std::size_t kRseqArraySize =
      rseq_available(RSEQ_AVAILABLE_QUERY_LIBC)
          // libc owns rseq. It automatically registers rseq at that start of
          // each thread.
          ? std::thread::hardware_concurrency()

          // Either rseq is unavailable, or it is unsupported by libc, in which
          // case rseq initialization can be thread-unsafe in the presence of
          // coroutines.
          : 0;

  return kRseqArraySize;
}

inline bool IsCpuIdValid(std::uint32_t cpu_id,
                         std::size_t array_size) noexcept {
  if (rseq_unlikely(cpu_id >= array_size)) {
    UASSERT_MSG(
        array_size == 0,
        "CPU count is not properly detected. Please report this issue."
        "You may want to set USERVER_DISABLE_RSEQ_ACCELERATION=ON cmake "
        "variable to overcome this error.");
    return false;
  }
  return true;
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif

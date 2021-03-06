#pragma once

/// @file userver/engine/io/common.hpp
/// @brief Common definitions

#include <cstddef>
#include <memory>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// File descriptor of an invalid pipe end.
static constexpr int kInvalidFd = -1;

/// Interface for readable streams
class ReadableBase {
 public:
  virtual ~ReadableBase() = default;

  /// Whether the stream is valid.
  virtual bool IsValid() const = 0;

  /// Suspends current task until the stream has data available.
  [[nodiscard]] virtual bool WaitReadable(Deadline) = 0;

  /// Receives at least one byte from the stream.
  [[nodiscard]] virtual size_t ReadSome(void* buf, size_t len,
                                        Deadline deadline) = 0;

  /// Receives exactly len bytes from the stream.
  /// @note Can return less than len if stream is closed by peer.
  [[nodiscard]] virtual size_t ReadAll(void* buf, size_t len,
                                       Deadline deadline) = 0;
};

using ReadableBasePtr = std::shared_ptr<ReadableBase>;

}  // namespace engine::io

USERVER_NAMESPACE_END

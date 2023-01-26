#pragma once

/// @file userver/engine/io/common.hpp
/// @brief Common definitions and base classes for stream like objects

#include <cstddef>
#include <memory>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// File descriptor of an invalid pipe end.
static constexpr int kInvalidFd = -1;

/// @ingroup userver_base_classes
///
/// Interface for readable streams
class ReadableBase {
 public:
  virtual ~ReadableBase();

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

/// @ingroup userver_base_classes
///
/// Interface for writable streams
class WritableBase {
 public:
  virtual ~WritableBase();

  /// Suspends current task until the data is available.
  [[nodiscard]] virtual bool WaitWriteable(Deadline deadline) = 0;

  /// @brief Sends exactly len bytes of buf.
  /// @note Can return less than len if stream is closed by peer.
  [[nodiscard]] virtual size_t WriteAll(const void* buf, size_t len,
                                        Deadline deadline) = 0;
};

/// @ingroup userver_base_classes
///
/// Interface for readable and writable streams
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class RwBase : public ReadableBase, public WritableBase {
 public:
  ~RwBase() override;
};

using ReadableBasePtr = std::shared_ptr<ReadableBase>;

}  // namespace engine::io

USERVER_NAMESPACE_END

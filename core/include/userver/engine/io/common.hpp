#pragma once

/// @file userver/engine/io/common.hpp
/// @brief Common definitions and base classes for stream like objects

#include <cstddef>
#include <memory>
#include <optional>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class ContextAccessor;
}

namespace engine::io {

/// File descriptor of an invalid pipe end.
inline constexpr int kInvalidFd = -1;

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

  /// Receives up to len (including zero) bytes from the stream.
  /// @returns filled-in optional on data presence (e.g. 0, 1, 2... bytes)
  ///  empty optional otherwise
  [[nodiscard]] virtual std::optional<size_t> ReadNoblock(void* buf,
                                                          size_t len) {
    (void)buf;
    (void)len;
    UINVARIANT(false, "not implemented yet");
    return {};
  }

  /// Receives at least one byte from the stream.
  [[nodiscard]] virtual size_t ReadSome(void* buf, size_t len,
                                        Deadline deadline) = 0;

  /// Receives exactly len bytes from the stream.
  /// @note Can return less than len if stream is closed by peer.
  [[nodiscard]] virtual size_t ReadAll(void* buf, size_t len,
                                       Deadline deadline) = 0;

  /// For internal use only
  impl::ContextAccessor* TryGetContextAccessor() { return ca_; }

 protected:
  void SetReadableContextAccessor(impl::ContextAccessor* ca) { ca_ = ca; }

 private:
  impl::ContextAccessor* ca_{nullptr};
};

/// IoData for vector send
struct IoData final {
  const void* data;
  size_t len;
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

  [[nodiscard]] virtual size_t WriteAll(std::initializer_list<IoData> list,
                                        Deadline deadline) {
    size_t result{0};
    for (const auto& io_data : list) {
      result += WriteAll(io_data.data, io_data.len, deadline);
    }
    return result;
  }

  /// For internal use only
  impl::ContextAccessor* TryGetContextAccessor() { return ca_; }

 protected:
  void SetWritableContextAccessor(impl::ContextAccessor* ca) { ca_ = ca; }

 private:
  impl::ContextAccessor* ca_{nullptr};
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

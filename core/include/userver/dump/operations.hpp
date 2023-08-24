#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <userver/dump/fwd.hpp>
#include <userver/dump/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// Indicates a failure reading or writing a dump. No further operations
/// should be performed with a failed dump.
class Error final : public std::runtime_error {
 public:
  explicit Error(std::string message) : std::runtime_error(message) {}
};

/// A general interface for binary data output
class Writer {
 public:
  virtual ~Writer() = default;

  /// @brief Writes binary data
  /// @details Calls ADL-found `Write(writer, data)`
  /// @throws `Error` and any user-thrown `std::exception`
  template <typename T>
  void Write(const T& data);

  /// @brief Must be called once all data has been written
  /// @warning This method must not be called from within `Write`/`Read`
  /// @throws `Error` on write operation failure
  virtual void Finish() = 0;

 protected:
  /// @brief Writes binary data
  /// @details Unlike `Write`, doesn't write the size of `data`
  /// @throws `Error` on write operation failure
  virtual void WriteRaw(std::string_view data) = 0;

  friend void WriteStringViewUnsafe(Writer& writer, std::string_view value);
};

/// A general interface for binary data input
class Reader {
 public:
  virtual ~Reader() = default;

  /// @brief Reads binary data
  /// @details Calls ADL-found `Read(reader, To<T>)`
  /// @throws `Error` and any user-thrown `std::exception`
  template <typename T>
  T Read();

  /// @brief Must be called once all data has been read
  /// @warning This method must not be called from within `Write`/`Read`
  /// @throws `Error` on read operation failure or if there is leftover data
  virtual void Finish() = 0;

 protected:
  /// @brief Reads binary data
  /// @note Invalidates the memory returned by the previous call of `ReadRaw`
  /// @note Normally, exactly `max_size` bytes is returned. On end-of-file,
  /// the amount of bytes returned can be less than `max_size`.
  /// @throws `Error` on read operation failure
  virtual std::string_view ReadRaw(std::size_t max_size) = 0;

  friend std::string_view ReadUnsafeAtMost(Reader& reader, std::size_t size);
};

namespace impl {

template <typename T>
void CallWrite(Writer& writer, const T& data) {
  Write(writer, data);
}

template <typename T>
// NOLINTNEXTLINE(readability-const-return-type)
T CallRead(Reader& reader, To<T> to) {
  return Read(reader, to);
}

}  // namespace impl

template <typename T>
void Writer::Write(const T& data) {
  if constexpr (kIsWritable<T>) {
    impl::CallWrite(*this, data);
  } else if constexpr (std::is_aggregate_v<T>) {
    static_assert(
        !sizeof(T),
        "Serialization is not implemented for this type. You "
        "either forgot to specialize IsDumpedAggregate for your type "
        "(see <userver/dump/aggregates.hpp>)"
        "or you've got a non-standard data type and need to implement "
        "`void Write(dump::Writer& writer, const T& data);` and put it "
        "in the namespace of `T` or in `dump`.");
  } else {
    static_assert(
        !sizeof(T),
        "You either forgot to `#include <userver/dump/common_containers.hpp>`, "
        "or you've got a non-standard data type and need to implement "
        "`void Write(dump::Writer& writer, const T& data);` and put it "
        "in the namespace of `T` or in `dump`.");
  }
}

template <typename T>
// NOLINTNEXTLINE(readability-const-return-type)
T Reader::Read() {
  if constexpr (kIsReadable<T>) {
    return impl::CallRead(*this, To<T>{});
  } else if constexpr (std::is_aggregate_v<T>) {
    static_assert(
        !sizeof(T),
        "Serialization is not implemented for this type. You "
        "either forgot to specialize IsDumpedAggregate for your type"
        "(see <userver/dump/aggregates.hpp>) "
        "or you've got a non-standard data type and need to implement "
        "`T Read(dump::Reader& reader, dump::To<T>);` and put it "
        "in the namespace of `T` or in `dump`.");
  } else {
    static_assert(
        !sizeof(T),
        "You either forgot to `#include <userver/dump/common_containers.hpp>`, "
        "or you've got a non-standard data type and need to implement"
        "`T Read(dump::Reader& reader, dump::To<T>);` and put it "
        "in the namespace of `T` or in `dump`.");
    return T{};
  }
}

}  // namespace dump

USERVER_NAMESPACE_END

#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <cache/dump/meta.hpp>

namespace cache::dump {

/// Indicates a failure reading or writing a cache dump. No further operations
/// should be performed with a failed dump.
class Error final : public std::runtime_error {
 public:
  explicit Error(std::string message) : std::runtime_error(message) {}
};

/// A marker type used in ADL-found `Read`
template <typename T>
struct To {};

/// A general interface for binary data output
class Writer {
 public:
  /// @brief Writes binary data
  /// @details Unlike `Write`, doesn't write the size of `data`
  /// @throws `Error` on write operation failure
  virtual void WriteRaw(std::string_view data) = 0;

  /// @brief Writes binary data
  /// @details Calls ADL-found `Write(writer, data)`
  /// @throws `Error` and any user-thrown `std::exception`
  template <typename T>
  void Write(const T& data);

  virtual ~Writer() = default;
};

/// A general interface for binary data input
class Reader {
 public:
  /// @brief Reads binary data
  /// @note Invalidates the memory returned by the previous call of `ReadRaw`
  /// @throws `Error` on read operation failure or a sudden end-of-input
  virtual std::string_view ReadRaw(std::size_t size) = 0;

  /// @brief Reads binary data
  /// @details Calls ADL-found `Read(reader, To<T>)`
  /// @throws `Error` and any user-thrown `std::exception`
  template <typename T>
  T Read();

  virtual ~Reader() = default;
};

namespace impl {

template <typename T>
void CallWrite(Writer& writer, const T& data) {
  Write(writer, data);
}

template <typename T>
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
        "either forgot to #include <cache/dump/aggregates.hpp>"
        "or you've got a non-standard data type and need to implement "
        "`void Write(cache::dump::Writer& writer, const T& data);` and put it "
        "in the namespace of `T` or in `cache::dump`.");
  } else {
    static_assert(
        !sizeof(T),
        "You either forgot to `#include <cache/dump/common_containers.hpp>`, "
        "or you've got a non-standard data type and need to implement "
        "`void Write(cache::dump::Writer& writer, const T& data);` and put it "
        "in the namespace of `T` or in `cache::dump`.");
  }
}

template <typename T>
T Reader::Read() {
  if constexpr (kIsReadable<T>) {
    return impl::CallRead(*this, To<T>{});
  } else if constexpr (std::is_aggregate_v<T>) {
    static_assert(
        !sizeof(T),
        "Serialization is not implemented for this type. You "
        "either forgot to #include <cache/dump/aggregates.hpp>"
        "or you've got a non-standard data type and need to implement "
        "`T Read(cache::dump::Reader& reader, cache::dump::To<T>);` and put it "
        "in the namespace of `T` or in `cache::dump`.");
  } else {
    static_assert(
        !sizeof(T),
        "You either forgot to `#include <cache/dump/common_containers.hpp>`, "
        "or you've got a non-standard data type and need to implement"
        "`T Read(cache::dump::Reader& reader, cache::dump::To<T>);` and put it "
        "in the namespace of `T` or in `cache::dump`.");
    return T{};
  }
}

}  // namespace cache::dump

#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <utils/fast_pimpl.hpp>

namespace cache::dump {

/// Indicates a failure reading or writing a cache dump. No further operations
/// should be performed with a failed dump.
class Error final : public std::runtime_error {
 public:
  explicit Error(std::string message);
};

/// A marker type used in ADL-found `Read`
template <typename T>
struct To {};

/// A handle to a cache dump file. File operations block the thread.
class Writer final {
 public:
  /// @brief Creates a new dump file and opens it
  /// @throws `Error` on a filesystem error
  explicit Writer(std::string path);

  /// @brief Writes binary data to the file
  /// @details Unlike `Write`, doesn't write the size of `data`
  /// @throws `Error` on a filesystem error
  void WriteRaw(std::string_view data);

  /// @brief Reads binary data from the file
  /// @details Calls ADL-found `Write(writer, data)`
  /// @throws `Error` on a filesystem error, or any user-thrown `std::exception`
  template <typename T>
  void Write(const T& data);

  /// @brief Must be called once all data has been written
  /// @warning This method is intended to only be called by `CacheUpdateTrait`
  /// @throws `Error` on a filesystem error
  void Finish();

  ~Writer();
  Writer(Writer&&) noexcept;
  Writer& operator=(Writer&&) noexcept;

 private:
  utils::FastPimpl<struct WriterImpl, 72, 8> impl_;
};

/// A handle to a cache dump file. File operations block the thread.
class Reader final {
 public:
  /// @brief Opens an existing dump file
  /// @throws `Error` on a filesystem error
  explicit Reader(std::string path);

  /// @brief Reads binary data from the file
  /// @note Invalidates the memory returned by the previous call of `ReadRaw`
  /// @throws `Error` on a filesystem error or a sudden end-of-file
  std::string_view ReadRaw(std::size_t size);

  /// @brief Reads binary data from the file
  /// @details Calls ADL-found `Read(reader, To<T>)`
  /// @throws `Error` on a filesystem error, or any user-thrown `std::exception`
  template <typename T>
  T Read();

  /// @brief Must be called once all data has been read
  /// @warning This method is intended to only be called by `CacheUpdateTrait`
  /// @throws `Error` on a filesystem error or if there is leftover data
  void Finish();

  ~Reader();
  Reader(Reader&&) noexcept;
  Reader& operator=(Reader&&) noexcept;

 private:
  utils::FastPimpl<struct ReaderImpl, 72, 8> impl_;
};

namespace impl {

template <typename T>
void CallWrite(Writer& writer, const T& data) {
  // If you got a compile-time error leading here, then you either forgot to
  // #include <cache/dump/common_containers.hpp>
  //
  // ...or you've got a non-standard data type and need to implement
  // void Write(cache::dump::Writer& writer, const T& data);
  //
  // and put it in the namespace of `T` or in `cache::dump`.
  Write(writer, data);
}

template <typename T>
T CallRead(Reader& reader, To<T> to) {
  // If you got a compile-time error leading here, then you either forgot to
  // #include <cache/dump/common_containers.hpp>
  //
  // ...or you've got a non-standard data type and need to implement
  // T Read(cache::dump::Reader& reader, cache::dump::To<T>);
  //
  // and put it in the namespace of `T` or in `cache::dump`.
  return Read(reader, to);
}

}  // namespace impl

template <typename T>
void Writer::Write(const T& data) {
  impl::CallWrite(*this, data);
}

template <typename T>
T Reader::Read() {
  return impl::CallRead<T>(*this, To<T>{});
}

}  // namespace cache::dump

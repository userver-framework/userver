#pragma once

/// @file userver/engine/io/buffered.hpp
/// @brief Buffered I/O wrappers

#include <functional>
#include <string>

#include <userver/compiler/select.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {
namespace impl {

class Buffer;

}  // namespace impl

class TerminatorNotFoundException : public IoException {
 public:
  TerminatorNotFoundException();
};

/// Wrapper for buffered input
class BufferedReader final {
 public:
  /// Creates a buffered reader with default buffer size.
  explicit BufferedReader(ReadableBasePtr source);

  /// Creates a buffered reader with specified initial buffer size.
  BufferedReader(ReadableBasePtr source, size_t buffer_size);

  ~BufferedReader();

  BufferedReader(BufferedReader&&) noexcept;
  BufferedReader& operator=(BufferedReader&&) noexcept;

  /// Whether the underlying source is valid.
  bool IsValid() const;

  /// Reads some bytes from the input stream.
  std::string ReadSome(size_t max_bytes, Deadline deadline = {});

  /// @brief Reads the exact number of bytes from the input stream.
  /// @note May return less bytes than requested in case of EOF.
  std::string ReadAll(size_t num_bytes, Deadline deadline = {});

  /// @brief Reads a line from the input, skipping empty lines.
  /// @note Does not return line terminators.
  std::string ReadLine(Deadline deadline = {});

  /// Reads the stream until the specified character of EOF is encountered.
  std::string ReadUntil(char terminator, Deadline deadline = {});

  /// @brief Reads the stream until the predicate returns `true`.
  /// @param pred predicate that will be called for each byte read and EOF.
  std::string ReadUntil(const std::function<bool(int)>& pred,
                        Deadline deadline = {});

  /// Reads one byte from the stream or reports an EOF (-1).
  int Getc(Deadline deadline = {});

  /// Returns the first byte of the buffer or EOF (-1).
  int Peek(Deadline deadline = {});

  /// Discards the specified number of bytes from the buffer.
  void Discard(size_t num_bytes, Deadline deadline = {});

 private:
  size_t FillBuffer(Deadline deadline);

  ReadableBasePtr source_;

  constexpr static std::size_t kBufferSize = compiler::SelectSize()  //
                                                 .For64Bit(40)
                                                 .For32Bit(20);
  constexpr static std::size_t kBufferAlignment = alignof(void*);
  utils::FastPimpl<impl::Buffer, kBufferSize, kBufferAlignment, true> buffer_;
};

}  // namespace engine::io

USERVER_NAMESPACE_END

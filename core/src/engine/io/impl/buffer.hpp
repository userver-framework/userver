#pragma once

#include <cstddef>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace engine::io::impl {

class Buffer {
 public:
  Buffer();
  explicit Buffer(size_t initial_size);

  Buffer(const Buffer&);
  Buffer(Buffer&&) noexcept = default;
  Buffer& operator=(const Buffer&);
  Buffer& operator=(Buffer&&) noexcept = default;

  size_t AvailableReadBytes() const;
  const char* ReadPtr() const;
  void ReportRead(size_t num_bytes);

  void Reserve(size_t num_bytes);
  size_t AvailableWriteBytes() const;
  char* WritePtr();
  void ReportWritten(size_t num_bytes);

 private:
  void Reallocate(size_t size_request);
  void Rebase();

  void Assign(const char* begin, const char* end);

  std::vector<char> data_;
  char* read_ptr_{nullptr};
  char* write_ptr_{nullptr};
};

}  // namespace engine::io::impl

USERVER_NAMESPACE_END

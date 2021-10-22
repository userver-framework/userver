#include <engine/io/impl/buffer.hpp>

#include <algorithm>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io::impl {
namespace {

constexpr size_t kDefaultBufferSize = 32 * 1024;
constexpr size_t kSizeGranularity = 4096;  // most frequent page size

constexpr size_t RoundedSize(size_t size) {
  return ((size + kSizeGranularity - 1) / kSizeGranularity) * kSizeGranularity;
}

}  // namespace

Buffer::Buffer() : Buffer(kDefaultBufferSize) {}

Buffer::Buffer(size_t initial_size)
    : data_(RoundedSize(initial_size)),
      read_ptr_(data_.data()),
      write_ptr_(read_ptr_) {}

Buffer::Buffer(const Buffer& other)
    : data_(RoundedSize(other.AvailableReadBytes())),
      read_ptr_(other.read_ptr_),
      write_ptr_(other.write_ptr_) {
  Rebase();
}

Buffer& Buffer::operator=(const Buffer& rhs) {
  if (this == &rhs) return *this;

  write_ptr_ = read_ptr_;  // clear
  Reserve(rhs.AvailableReadBytes());
  read_ptr_ = rhs.read_ptr_;
  write_ptr_ = rhs.write_ptr_;
  Rebase();
  return *this;
}

size_t Buffer::AvailableReadBytes() const {
  return write_ptr_ ? write_ptr_ - read_ptr_ : 0;
}

const char* Buffer::ReadPtr() const { return read_ptr_; }

void Buffer::ReportRead(size_t num_bytes) {
  UASSERT(num_bytes <= AvailableReadBytes());
  read_ptr_ += num_bytes;
  if (num_bytes && !AvailableReadBytes()) Rebase();
}

size_t Buffer::AvailableWriteBytes() const {
  return write_ptr_ ? data_.data() + data_.size() - write_ptr_ : 0;
}

void Buffer::Reserve(size_t num_bytes) {
  if (num_bytes + AvailableReadBytes() > data_.size()) {
    Reallocate(num_bytes + AvailableReadBytes());
  }
  if (num_bytes > AvailableReadBytes() + AvailableWriteBytes()) Rebase();
}

char* Buffer::WritePtr() { return write_ptr_; }

void Buffer::ReportWritten(size_t num_bytes) {
  UASSERT(num_bytes <= AvailableWriteBytes());
  write_ptr_ += num_bytes;
}

void Buffer::Reallocate(size_t size_request) {
  UASSERT(size_request > data_.size());

  std::vector<char> tmp(RoundedSize(size_request));
  tmp.swap(data_);  // does not invalidate ptrs
  Rebase();
}

void Buffer::Rebase() {
  UASSERT(!data_.empty());
  UASSERT(read_ptr_ != data_.data());

  size_t read_bytes = AvailableReadBytes();
  if (read_bytes) {
    UASSERT(read_bytes <= data_.size());
    std::copy(read_ptr_, write_ptr_, data_.data());
  }

  read_ptr_ = data_.data();
  write_ptr_ = data_.data() + read_bytes;
}

}  // namespace engine::io::impl

USERVER_NAMESPACE_END

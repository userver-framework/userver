#include <cache/dump/operations_mock.hpp>

#include <utility>

#include <fmt/format.h>

namespace cache::dump {

MockWriter::MockWriter() = default;

void MockWriter::WriteRaw(std::string_view data) { data_.append(data); }

std::string MockWriter::Extract() && { return std::move(data_); }

MockReader::MockReader(std::string data)
    : data_(std::move(data)), unread_data_(data_) {}

std::string_view MockReader::ReadRaw(std::size_t size) {
  std::string_view result(unread_data_.substr(0, size));
  if (result.size() < size) {
    throw Error(fmt::format(
        "Unexpected end-of-file while trying to read from the dump: "
        "file-size={}, position={}, requested-size={}",
        data_.size(), data_.size() - unread_data_.size(), size));
  }
  unread_data_ = unread_data_.substr(size);
  return result;
}

void MockReader::Finish() {
  if (!unread_data_.empty()) {
    throw Error(fmt::format(
        "Unexpected extra data at the end of the dump: file-size={}, "
        "position={}, unread-size={}",
        data_.size(), data_.size() - unread_data_.size(), unread_data_.size()));
  }
}

}  // namespace cache::dump

#include <userver/dump/operations_mock.hpp>

#include <algorithm>
#include <utility>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace dump {

MockWriter::MockWriter() = default;

void MockWriter::WriteRaw(std::string_view data) { data_.append(data); }

void MockWriter::Finish() {
  // nothing to do
}

std::string MockWriter::Extract() && { return std::move(data_); }

MockReader::MockReader(std::string data)
    : data_(std::move(data)), unread_data_(data_) {}

std::string_view MockReader::ReadRaw(std::size_t max_size) {
  const auto result_size = std::min(max_size, unread_data_.size());
  const auto result = unread_data_.substr(0, result_size);
  unread_data_ = unread_data_.substr(result_size);
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

}  // namespace dump

USERVER_NAMESPACE_END

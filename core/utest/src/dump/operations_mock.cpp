#include <userver/dump/operations_mock.hpp>

#include <algorithm>
#include <utility>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

MockWriter::MockWriter() = default;

void MockWriter::WriteRaw(std::string_view data) { data_.append(data); }

void MockWriter::Finish() {
    // nothing to do
}

std::string MockWriter::Extract() && { return std::move(data_); }

MockReader::MockReader(std::string data) : data_(std::move(data)) {}

std::string_view MockReader::ReadRaw(std::size_t max_size) {
    UASSERT(pos_ <= data_.size());
    const auto result_size = std::min(max_size, data_.size() - pos_);
    const auto result = std::string_view{data_}.substr(pos_, result_size);
    pos_ += result_size;
    return result;
}

void MockReader::BackUp(std::size_t size) {
    UASSERT_MSG(pos_ <= data_.size(), "Trying to BackUp more bytes than returned by the last ReadRaw");
    pos_ -= size;
}

void MockReader::Finish() {
    if (pos_ != data_.size()) {
        throw Error(fmt::format(
            "Unexpected extra data at the end of the dump: file-size={}, position={}, unread-size={}",
            data_.size(),
            pos_,
            data_.size() - pos_
        ));
    }
}

}  // namespace dump

USERVER_NAMESPACE_END

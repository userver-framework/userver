#include <userver/kafka/exceptions.hpp>
#include <userver/kafka/headers.hpp>

#include <librdkafka/rdkafka.h>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

bool HeaderView::operator==(const HeaderView& other) const {
    return std::tie(name, value) == std::tie(other.name, other.value);
}

bool operator==(const HeaderView& lhs, const OwningHeader& rhs) {
    return std::tie(lhs.name, lhs.value) == std::make_tuple(rhs.GetName(), rhs.GetValue());
}

bool operator==(const OwningHeader& lhs, const HeaderView& rhs) { return (rhs == lhs); }

OwningHeader::OwningHeader(HeaderView header) : name_{header.name}, value_{header.value} {}

OwningHeader::OwningHeader(std::string name, std::string value) : name_{std::move(name)}, value_{std::move(value)} {}

const std::string& OwningHeader::GetName() const noexcept { return name_; }

std::string_view OwningHeader::GetValue() const noexcept { return value_; }

HeadersIterator::HeadersIterator(const rd_kafka_headers_t* headers, std::size_t index)
    : headers_{headers}, index_{index} {}

HeadersIterator& HeadersIterator::operator++() {
    ++index_;

    return *this;
}

HeadersIterator HeadersIterator::operator++(int) {
    HeadersIterator ret{headers_, index_};
    ++index_;

    return ret;
}

auto HeadersIterator::operator*() const -> reference {
    const char* name{nullptr};
    const void* value{nullptr};
    std::size_t value_size{};

    const auto err = rd_kafka_header_get_all(headers_, index_, &name, &value, &value_size);

    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        throw ParseHeadersException{rd_kafka_err2str(err)};
    }

    return HeaderView{name, std::string_view{static_cast<const char*>(value), value_size}};
}

bool HeadersIterator::operator==(const HeadersIterator& other) const {
    return std::tie(headers_, index_) == std::tie(other.headers_, other.index_);
}

bool HeadersIterator::operator!=(const HeadersIterator& other) const { return !(*this == other); }

HeadersReader::HeadersReader(const rd_kafka_headers_t* headers) : headers_{headers}, size_{size()} {}

std::size_t HeadersReader::size() const noexcept {
    if (!headers_) {
        return 0;
    }

    return rd_kafka_header_cnt(headers_);
}

HeadersIterator HeadersReader::begin() const { return HeadersIterator{headers_, 0}; }

HeadersIterator HeadersReader::end() const { return HeadersIterator{headers_, size_}; }

}  // namespace kafka

USERVER_NAMESPACE_END

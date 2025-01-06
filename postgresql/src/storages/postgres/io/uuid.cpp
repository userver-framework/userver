#include <userver/storages/postgres/io/uuid.hpp>

#include <boost/uuid/uuid.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

void BufferFormatter<boost::uuids::uuid>::operator()(const UserTypes&, std::vector<char>& buf) const {
    std::copy(value.begin(), value.end(), std::back_inserter(buf));
}

void BufferFormatter<boost::uuids::uuid>::operator()(const UserTypes&, std::string& buf) const {
    std::copy(value.begin(), value.end(), std::back_inserter(buf));
}

void BufferParser<boost::uuids::uuid>::operator()(const FieldBuffer& buf) {
    if (buf.length != boost::uuids::uuid::static_size()) {
        throw InvalidInputBufferSize{fmt::format("Invalid buffer size {} for an uuid type", buf.length)};
    }
    std::copy(buf.buffer, buf.buffer + buf.length, value.begin());
}
}  // namespace storages::postgres::io

USERVER_NAMESPACE_END

#pragma once

#include <cstddef>
#include <string>

#include <userver/storages/redis/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class ZaddReply final {
public:
    explicit ZaddReply(size_t value);

    static ZaddReply Parse(ReplyData&& reply_data, const std::string& request_description = {});

    size_t GetCount() const;

private:
    size_t value_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END

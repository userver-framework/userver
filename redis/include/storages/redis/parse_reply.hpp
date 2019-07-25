#pragma once

#include <string>

#include <redis/types.hpp>

namespace storages {
namespace redis {

template <typename Result, typename ReplyType = Result>
ReplyType ParseReply(const ::redis::ReplyPtr& reply,
                     const std::string& request_description = {});

}  // namespace redis
}  // namespace storages

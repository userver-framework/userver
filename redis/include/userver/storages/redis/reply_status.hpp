#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// Redis reply status
enum class ReplyStatus {
    kOk = 0,
    kInputOutputError,
    kOtherError,
    kEndOfFileError,
    kProtocolError,
    kOutOfMemoryError,
    kTimeoutError,
};

}  // namespace storages::redis

#ifdef USERVER_FEATURE_LEGACY_REDIS_NAMESPACE
namespace redis {
using storages::redis::ReplyStatus;
}
#endif

USERVER_NAMESPACE_END

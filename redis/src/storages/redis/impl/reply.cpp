#include <userver/storages/redis/reply.hpp>

#include <sstream>
#include <string>

#include <hiredis/hiredis.h>
#include <boost/algorithm/string/predicate.hpp>

#include <userver/storages/redis/exception.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

ReplyData::ReplyData(const redisReply* reply) {
    UASSERT(data_.index() == utils::UnderlyingValue(Type::kNoReply));
    if (!reply) return;

    switch (reply->type) {
        case REDIS_REPLY_STRING:
            data_ = String(reply->str, reply->len);
            UASSERT(data_.index() == utils::UnderlyingValue(Type::kString));
            break;
        case REDIS_REPLY_ARRAY:
            data_ = Array(reply->element, reply->element + reply->elements);
            UASSERT(data_.index() == utils::UnderlyingValue(Type::kArray));
            break;
        case REDIS_REPLY_INTEGER:
            data_ = reply->integer;
            UASSERT(data_.index() == utils::UnderlyingValue(Type::kInteger));
            break;
        case REDIS_REPLY_NIL:
            data_ = Nil{};
            UASSERT(data_.index() == utils::UnderlyingValue(Type::kNil));
            break;
        case REDIS_REPLY_STATUS:
            data_ = Status(reply->str, reply->len);
            UASSERT(data_.index() == utils::UnderlyingValue(Type::kStatus));
            break;
        case REDIS_REPLY_ERROR:
            data_ = Error(reply->str, reply->len);
            UASSERT(data_.index() == utils::UnderlyingValue(Type::kError));
            break;
        default:
            data_ = NoReply{};
            UASSERT(data_.index() == utils::UnderlyingValue(Type::kNoReply));
            break;
    }
}

ReplyData::ReplyData(Array&& array) : data_(std::move(array)) {}

ReplyData::ReplyData(std::string s) : data_(String(std::move(s))) {}

ReplyData::ReplyData(int value) : data_(Integer{value}) {}

ReplyData ReplyData::CreateError(std::string&& error_msg) {
    ReplyData data;
    data.data_ = Error(std::move(error_msg));
    UASSERT(data.data_.index() == utils::UnderlyingValue(Type::kError));
    return data;
}

ReplyData ReplyData::CreateStatus(std::string&& status_msg) {
    ReplyData data;
    data.data_ = Status(std::move(status_msg));
    UASSERT(data.data_.index() == utils::UnderlyingValue(Type::kStatus));
    return data;
}

ReplyData ReplyData::CreateNil() {
    ReplyData data;
    data.data_ = Nil{};
    UASSERT(data.data_.index() == utils::UnderlyingValue(Type::kNil));
    return data;
}

std::string ReplyData::GetTypeString() const { return TypeToString(GetType()); }

std::string ReplyData::ToDebugString() const {
    switch (GetType()) {
        case ReplyData::Type::kNoReply:
            return {};
        case ReplyData::Type::kNil:
            return "(nil)";
        case ReplyData::Type::kString:
            return GetString();
        case ReplyData::Type::kStatus:
            return GetStatus();
        case ReplyData::Type::kError:
            return GetError();
        case ReplyData::Type::kInteger:
            return std::to_string(GetInt());
        case ReplyData::Type::kArray: {
            std::ostringstream os;
            const auto& array = GetArray();
            os << "[";
            for (size_t i = 0; i < array.size(); i++) {
                if (i) os << ", ";
                os << array[i].ToDebugString();
            }
            os << "]";
            return os.str();
        }
        default:
            return "(unknown type)";
    }
}

ReplyData::MovableKeyValues ReplyData::GetMovableKeyValues() {
    if (!IsArray())
        throw ParseReplyException("Incorrect ReplyData type: expected kArray, found " + TypeToString(GetType()));
    if (GetArray().size() & 1) throw ParseReplyException("Array size is odd: " + std::to_string(GetArray().size()));
    for (const auto& elem : GetArray())
        if (!elem.IsString()) throw ParseReplyException("Non-string element (" + elem.GetTypeString() + ')');
    return ReplyData::MovableKeyValues(GetArray());
}

std::string ReplyData::TypeToString(Type type) {
    switch (type) {
        case Type::kNoReply:
            return "kNoReply";
        case Type::kString:
            return "kString";
        case Type::kArray:
            return "kArray";
        case Type::kInteger:
            return "kInteger";
        case Type::kNil:
            return "kNil";
        case Type::kStatus:
            return "kStatus";
        case Type::kError:
            return "kError";
    };

    return "Unknown (" + std::to_string(utils::UnderlyingValue(type)) + ")";
}

std::size_t ReplyData::GetSize() const noexcept {
    std::size_t sum = 0;

    switch (GetType()) {
        case Type::kNoReply:
            return 1;

        case Type::kArray:
            for (const auto& data : GetArray()) sum += data.GetSize();
            return sum;

        case Type::kInteger:
            return sizeof(Integer);

        case Type::kNil:
            return 1;

        case Type::kString:
            return GetString().size();
        case Type::kStatus:
            return GetStatus().size();
        case Type::kError:
            return GetError().size();
    }
    return 1;
}

bool ReplyData::IsUnusableInstanceError() const {
    if (IsError()) {
        const auto& msg = GetError();

        if (boost::starts_with(msg.c_str(), "MASTERDOWN ")) return true;
        if (boost::starts_with(msg.c_str(), "LOADING ")) return true;
    }

    return false;
}

bool ReplyData::IsReadonlyError() const {
    if (IsError()) {
        const auto& msg = GetError();

        if (boost::starts_with(msg.c_str(), "READONLY ")) return true;
    }

    return false;
}

bool ReplyData::IsUnknownCommandError() const {
    if (IsError()) {
        const auto& msg = GetError();

        if (boost::starts_with(msg.c_str(), "ERR unknown command ")) return true;
    }

    return false;
}

void ReplyData::ExpectType(ReplyData::Type type, const std::string& request_description) const {
    if (GetType() != type) ThrowUnexpectedReplyType(type, request_description);
}

void ReplyData::ExpectString(const std::string& request_description) const {
    ExpectType(ReplyData::Type::kString, request_description);
}

void ReplyData::ExpectArray(const std::string& request_description) const {
    ExpectType(ReplyData::Type::kArray, request_description);
}

void ReplyData::ExpectInt(const std::string& request_description) const {
    ExpectType(ReplyData::Type::kInteger, request_description);
}

void ReplyData::ExpectNil(const std::string& request_description) const {
    ExpectType(ReplyData::Type::kNil, request_description);
}

void ReplyData::ExpectStatus(const std::string& request_description) const {
    ExpectType(ReplyData::Type::kStatus, request_description);
}

void ReplyData::ExpectStatusEqualTo(const std::string& expected_status_str, const std::string& request_description)
    const {
    ExpectStatus(request_description);
    if (GetStatus() != expected_status_str) {
        throw ParseReplyException(
            "Unexpected redis reply to '" + request_description + "' request: expected status=" + expected_status_str +
            ", got status=" + GetStatus()
        );
    }
}

void ReplyData::ExpectError(const std::string& request_description) const {
    ExpectType(ReplyData::Type::kError, request_description);
}

[[noreturn]] void ReplyData::ThrowUnexpectedReplyType(ReplyData::Type expected, const std::string& request_description)
    const {
    throw ParseReplyException(
        "Unexpected redis reply type to '" + request_description + "' request: expected " +
        ReplyData::TypeToString(expected) + ", got type=" + GetTypeString() + " data=" + ToDebugString()
    );
}

Reply::Reply(std::string command, ReplyData&& reply_data, ReplyStatus reply_status)
    : cmd(std::move(command)), data(std::move(reply_data)), status(reply_status) {
    UASSERT_MSG(
        !IsOk() || !data.IsError(),
        fmt::format(
            "For command '{}' the ReplyData contains error='{}' that missmatch Reply status kOk", cmd, data.GetError()
        )
    );
}

bool Reply::IsOk() const { return status == ReplyStatus::kOk; }

bool Reply::IsLoggableError() const {
    /* In case of not loggable error just filter it out */
    return !IsOk();
}

bool Reply::IsUnusableInstanceError() const { return data.IsUnusableInstanceError(); }

bool Reply::IsReadonlyError() const { return data.IsReadonlyError(); }

bool Reply::IsUnknownCommandError() const { return data.IsUnknownCommandError(); }

const logging::LogExtra& Reply::GetLogExtra() const { return log_extra; }

void Reply::FillSpanTags(tracing::Span& span) const {
    span.AddTag(tracing::kDatabaseType, tracing::kDatabaseRedisType);
    span.AddTag(tracing::kPeerAddress, server);
}

void Reply::ExpectIsOk(const std::string& request_description) const {
    if (!IsOk()) {
        throw RequestFailedException(GetRequestDescription(request_description), status);
    }
}

const std::string& Reply::GetRequestDescription(const std::string& request_description) const {
    return request_description.empty() ? cmd : request_description;
}

}  // namespace storages::redis

USERVER_NAMESPACE_END

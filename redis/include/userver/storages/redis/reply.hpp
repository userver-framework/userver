#pragma once

#include <string>
#include <variant>
#include <vector>

#include <userver/logging/log_extra.hpp>
#include <userver/utils/assert.hpp>

#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/reply_fwd.hpp>
#include <userver/storages/redis/reply_status.hpp>

struct redisReply;

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class ReplyData final {
public:
    using Array = std::vector<ReplyData>;

    enum class Type {
        kNoReply,
        kString,
        kArray,
        kInteger,
        kNil,
        kStatus,
        kError,
    };

    class MovableKeyValues final {
    public:
        class View final {
        public:
            View(ReplyData& key_data, ReplyData& value_data) noexcept : key_data_(key_data), value_data_(value_data) {}

            std::string& Key() noexcept { return key_data_.GetString(); }
            std::string& Value() noexcept { return value_data_.GetString(); }

        private:
            ReplyData& key_data_;
            ReplyData& value_data_;
        };

        class Iterator final {
        public:
            constexpr Iterator(Array& array, std::size_t index) noexcept : array_(array), index_(index) {}
            Iterator& operator++() noexcept {
                ++index_;
                return *this;
            }
            bool operator!=(const Iterator& r) const noexcept { return index_ != r.index_; }
            View operator*() noexcept { return {array_[index_ * 2], array_[index_ * 2 + 1]}; }

        private:
            Array& array_;
            std::size_t index_;
        };

        explicit MovableKeyValues(Array& array) noexcept : array_(array) {}

        Iterator begin() const noexcept { return {array_, 0}; }
        Iterator end() const noexcept { return {array_, size()}; }

        std::size_t size() const noexcept { return array_.size() / 2; }

    private:
        Array& array_;
    };

    MovableKeyValues GetMovableKeyValues();

    ReplyData(const redisReply* reply);
    ReplyData(Array&& array);
    ReplyData(std::string s);
    ReplyData(int value);
    static ReplyData CreateError(std::string&& error_msg);
    static ReplyData CreateStatus(std::string&& status_msg);
    static ReplyData CreateNil();

    explicit operator bool() const noexcept { return GetType() != Type::kNoReply; }

    Type GetType() const noexcept {
        UASSERT(!data_.valueless_by_exception());
        return Type(data_.index());
    }
    std::string GetTypeString() const;

    inline bool IsString() const noexcept { return GetType() == Type::kString; }
    inline bool IsArray() const noexcept { return GetType() == Type::kArray; }
    inline bool IsInt() const noexcept { return GetType() == Type::kInteger; }
    inline bool IsNil() const noexcept { return GetType() == Type::kNil; }
    inline bool IsStatus() const noexcept { return GetType() == Type::kStatus; }
    inline bool IsError() const noexcept { return GetType() == Type::kError; }
    bool IsUnusableInstanceError() const;
    bool IsReadonlyError() const;
    bool IsUnknownCommandError() const;

    bool IsErrorMoved() const { return IsError() && !GetError().compare(0, 6, "MOVED "); }

    bool IsErrorAsk() const { return IsError() && !GetError().compare(0, 4, "ASK "); }

    bool IsErrorClusterdown() const { return IsError() && !GetError().compare(0, 12, "CLUSTERDOWN "); }

    const std::string& GetString() const {
        UASSERT(IsString());
        return std::get_if<String>(&data_)->GetUnderlying();
    }

    std::string& GetString() {
        UASSERT(IsString());
        return std::get_if<String>(&data_)->GetUnderlying();
    }

    const Array& GetArray() const {
        UASSERT(IsArray());
        return *std::get_if<Array>(&data_);
    }

    Array& GetArray() {
        UASSERT(IsArray());
        return *std::get_if<Array>(&data_);
    }

    std::int64_t GetInt() const {
        UASSERT(IsInt());
        return *std::get_if<Integer>(&data_);
    }

    const std::string& GetStatus() const {
        UASSERT(IsStatus());
        return std::get_if<Status>(&data_)->GetUnderlying();
    }

    std::string& GetStatus() {
        UASSERT(IsStatus());
        return std::get_if<Status>(&data_)->GetUnderlying();
    }

    const std::string& GetError() const {
        UASSERT(IsError());
        return std::get_if<Error>(&data_)->GetUnderlying();
    }

    std::string& GetError() {
        UASSERT(IsError());
        return std::get_if<Error>(&data_)->GetUnderlying();
    }

    std::size_t GetSize() const noexcept;

    std::string ToDebugString() const;
    static std::string TypeToString(Type type);

    void ExpectType(ReplyData::Type type, const std::string& request_description = {}) const;

    void ExpectString(const std::string& request_description = {}) const;
    void ExpectArray(const std::string& request_description = {}) const;
    void ExpectInt(const std::string& request_description = {}) const;
    void ExpectNil(const std::string& request_description = {}) const;
    void ExpectStatus(const std::string& request_description = {}) const;
    void ExpectStatusEqualTo(const std::string& expected_status_str, const std::string& request_description = {}) const;
    void ExpectError(const std::string& request_description = {}) const;

private:
    ReplyData() = default;

    [[noreturn]] void ThrowUnexpectedReplyType(ReplyData::Type expected, const std::string& request_description) const;

    struct NoReply {};
    using String = utils::StrongTypedef<class StringTag, std::string>;
    using Integer = std::int64_t;
    struct Nil {};
    using Status = utils::StrongTypedef<class StatusTag, std::string>;
    using Error = utils::StrongTypedef<class ErrorTag, std::string>;

    // Matches `enum class Type`
    std::variant<NoReply, String, Array, Integer, Nil, Status, Error> data_{};
};

class Reply final {
public:
    Reply(std::string command, ReplyData&& reply_data, ReplyStatus reply_status = ReplyStatus::kOk);

    std::string server;
    ServerId server_id;
    const std::string cmd;
    ReplyData data;
    const ReplyStatus status;
    double time = 0.0;
    logging::LogExtra log_extra;

    operator bool() const { return IsOk(); }

    std::string_view GetStatusString() const {
        return (!IsOk() && data.IsError() ? data.GetError() : std::string_view{});
    }

    bool IsOk() const;
    bool IsLoggableError() const;
    bool IsUnusableInstanceError() const;
    bool IsReadonlyError() const;
    bool IsUnknownCommandError() const;
    const logging::LogExtra& GetLogExtra() const;
    void FillSpanTags(tracing::Span& span) const;

    void ExpectIsOk(const std::string& request_description = {}) const;

    const std::string& GetRequestDescription(const std::string& request_description) const;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END

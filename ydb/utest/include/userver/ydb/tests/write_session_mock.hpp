#pragma once

#include <gmock/gmock.h>

#include <ydb-cpp-sdk/client/topic/write_session.h>

/// @file userver/ydb/tests/write_session_mock.hpp
/// @brief GMock-based mock for NYdb::NTopic::IWriteSession, for use in unit tests

USERVER_NAMESPACE_BEGIN

namespace ydb::tests {

/// @brief GMock-based mock for IWriteSession, for use in unit tests
class WriteSessionMock : public NYdb::NTopic::IWriteSession {
public:
    MOCK_METHOD(NThreading::TFuture<void>, WaitEvent, (), (override));
    MOCK_METHOD(std::optional<NYdb::NTopic::TWriteSessionEvent::TEvent>, GetEvent, (bool), (override));
    MOCK_METHOD(
        std::vector<NYdb::NTopic::TWriteSessionEvent::TEvent>,
        GetEvents,
        (bool, std::optional<std::size_t>),
        (override)
    );
    MOCK_METHOD(NThreading::TFuture<uint64_t>, GetInitSeqNo, (), (override));
    MOCK_METHOD(
        void,
        Write,
        (NYdb::NTopic::TContinuationToken&&, NYdb::NTopic::TWriteMessage&&, NYdb::TTransactionBase*),
        (override)
    );
    MOCK_METHOD(
        void,
        Write,
        (NYdb::NTopic::TContinuationToken&&, std::string_view, std::optional<uint64_t>, std::optional<TInstant>),
        (override)
    );
    MOCK_METHOD(
        void,
        WriteEncoded,
        (NYdb::NTopic::TContinuationToken&&, NYdb::NTopic::TWriteMessage&&, NYdb::TTransactionBase*),
        (override)
    );
    MOCK_METHOD(
        void,
        WriteEncoded,
        (NYdb::NTopic::TContinuationToken&&,
         std::string_view,
         NYdb::NTopic::ECodec,
         uint32_t,
         std::optional<uint64_t>,
         std::optional<TInstant>),
        (override)
    );
    MOCK_METHOD(bool, Close, (TDuration), (override));
    MOCK_METHOD(NYdb::NTopic::TWriterCounters::TPtr, GetCounters, (), (override));
};

}  // namespace ydb::tests

USERVER_NAMESPACE_END

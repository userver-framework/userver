#pragma once

/// @file userver/ydb/tests/topic_writer_mock.hpp
/// @brief GMock-based mock for TopicWriterBase, for use in unit tests

#include <gmock/gmock.h>

#include <string>

#include <userver/ydb/topic_writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::tests {

/// @brief GMock-based mock for TopicWriterBase, for use in unit tests
///
/// Use this class to create mock for topic writer.
///
/// @snippet ydb/tests/topic_writer_mock_test.cpp example1
class TopicWriterMock : public TopicWriterBase {
public:
    /// @brief Mock implementation of WriteMessage
    MOCK_METHOD(
        ydb::TopicWriteResult,
        WriteMessage,
        (std::string message, const ydb::TopicWriterMetadata& meta),
        (override)
    );

    ~TopicWriterMock() override = default;
};

}  // namespace ydb::tests

USERVER_NAMESPACE_END

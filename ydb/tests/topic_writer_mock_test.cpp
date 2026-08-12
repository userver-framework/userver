#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/utest/utest.hpp>
#include <userver/ydb/tests/topic_writer_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

/// [example1]
UTEST(TopicWriterMockFixture, Example) {
    ydb::tests::TopicWriterMock writer_mock;
    EXPECT_CALL(writer_mock, WriteMessage(::testing::_, ::testing::_))
        .WillOnce([](std::string, const ydb::TopicWriterMetadata&) {
            return ydb::TopicWriteResult{ydb::TopicWriteStatus::kOk};
        });

    // Execute code
    auto result = writer_mock.WriteMessage("test message", {});
    EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
}
/// [example1]

}  // namespace tests

USERVER_NAMESPACE_END

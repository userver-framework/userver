#include <gmock/gmock.h>

#include <userver/s3api/utest/client_gmock.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utest/utest.hpp>

#include <s3api_client.hpp>

/// [client_tests]
UTEST(S3ClientTest, HappyPath) {
    auto mock = std::make_shared<s3api::GMockClient>();
    EXPECT_CALL(
        *mock,
        PutObject(
            std::string_view{"path/to/object"}, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_
        )
    )
        .Times(1)
        .WillOnce(::testing::Return("OK"));
    EXPECT_CALL(*mock, GetObject(std::string_view{"path/to/object"}, ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(std::string{"some data"}));

    samples::DoVeryImportantThingsInS3(mock);
}
/// [client_tests]

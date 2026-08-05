#include <s3api/s3_connection.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

namespace {

TEST(S3ConnectionHostHeader, BareHostWithBucket) {
    // Продовый случай: api_url_ — голый хост, virtual-host адресация.
    EXPECT_EQ(S3Connection::MakeHostHeader("s3.mds.yandex.net", "mybucket"), "mybucket.s3.mds.yandex.net");
}

TEST(S3ConnectionHostHeader, BareHostWithoutBucket) {
    EXPECT_EQ(S3Connection::MakeHostHeader("s3.mds.yandex.net", ""), "s3.mds.yandex.net");
}

TEST(S3ConnectionHostHeader, LocalhostWithSchemeAndBucket) {
    // Тестовый endpoint на mockserver: схему и путь отбрасываем, но bucket
    // сохраняем — mock определяет его по Host (virtual-host).
    EXPECT_EQ(S3Connection::MakeHostHeader("http://localhost:41871/s3mds", "bucket"), "bucket.localhost:41871");
}

TEST(S3ConnectionHostHeader, LocalhostWithSchemeAndPathWithoutBucket) {
    // Bucket пуст, имя мока зашито в путь endpoint (кейс fintech): остаётся
    // только authority.
    EXPECT_EQ(S3Connection::MakeHostHeader("http://localhost:41507/risk-collection-ui", ""), "localhost:41507");
}

TEST(S3ConnectionHostHeader, RemoteHostWithScheme) {
    EXPECT_EQ(S3Connection::MakeHostHeader("http://s3.example.com", "bucket"), "bucket.s3.example.com");
}

TEST(S3ConnectionHostHeader, RemoteHostWithSchemeAndPath) {
    EXPECT_EQ(S3Connection::MakeHostHeader("https://s3.example.com/prefix", "bucket"), "bucket.s3.example.com");
}

}  // namespace

}  // namespace s3api

USERVER_NAMESPACE_END

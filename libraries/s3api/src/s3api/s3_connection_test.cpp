#include <string>
#include <string_view>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

// Внутренняя функция из s3_connection.cpp с внешним связыванием.
std::string MakeHostHeader(std::string_view api_url, std::string_view bucket);

namespace {

TEST(S3ConnectionHostHeader, BareHostWithBucket) {
    // Продовый случай: api_url_ — голый хост, virtual-host адресация.
    EXPECT_EQ(MakeHostHeader("s3.mds.yandex.net", "mybucket"), "mybucket.s3.mds.yandex.net");
}

TEST(S3ConnectionHostHeader, BareHostWithoutBucket) {
    EXPECT_EQ(MakeHostHeader("s3.mds.yandex.net", ""), "s3.mds.yandex.net");
}

TEST(S3ConnectionHostHeader, LocalhostWithSchemeAndBucket) {
    // Тестовый endpoint на mockserver: схему и путь отбрасываем, но bucket
    // сохраняем — mock определяет его по Host (virtual-host).
    EXPECT_EQ(MakeHostHeader("http://localhost:41871/s3mds", "bucket"), "bucket.localhost:41871");
}

TEST(S3ConnectionHostHeader, LocalhostWithSchemeAndPathWithoutBucket) {
    // Bucket пуст, имя мока зашито в путь endpoint (кейс fintech): остаётся
    // только authority.
    EXPECT_EQ(MakeHostHeader("http://localhost:41507/risk-collection-ui", ""), "localhost:41507");
}

TEST(S3ConnectionHostHeader, RemoteHostWithScheme) {
    EXPECT_EQ(MakeHostHeader("http://s3.example.com", "bucket"), "bucket.s3.example.com");
}

TEST(S3ConnectionHostHeader, RemoteHostWithSchemeAndPath) {
    EXPECT_EQ(MakeHostHeader("https://s3.example.com/prefix", "bucket"), "bucket.s3.example.com");
}

}  // namespace

}  // namespace s3api

USERVER_NAMESPACE_END

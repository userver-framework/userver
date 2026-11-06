#include <userver/s3api/authenticators/signature_v4.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/s3api/models/request.hpp>
#include <userver/utils/mock_now.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

namespace {

// Test vectors from
// https://docs.aws.amazon.com/AmazonS3/latest/developerguide/sig-v4-header-based-auth.html
constexpr std::string_view kAccessKey = "AKIAIOSFODNN7EXAMPLE";
constexpr std::string_view kSecretKey = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";
constexpr std::string_view kRegion = "us-east-1";
constexpr std::string_view kBucket = "examplebucket";
constexpr std::string_view kVirtualHost = "examplebucket.s3.amazonaws.com";

constexpr std::string_view kEmptyPayloadHash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

// Fri, 24 May 2013 00:00:00 GMT
constexpr time_t kMockedNowEpoch = 1369353600;

SignatureV4 MakeAuthenticator() {
    return SignatureV4{std::string{kAccessKey}, Secret{std::string{kSecretKey}}, std::string{kRegion}};
}

Request MakeRequest(clients::http::HttpMethod method, std::string req) {
    utils::datetime::MockNowSet(std::chrono::system_clock::from_time_t(kMockedNowEpoch));

    Request request;
    request.method = method;
    request.bucket = kBucket;
    request.req = std::move(req);
    request.headers[USERVER_NAMESPACE::http::headers::kHost] = std::string{kVirtualHost};
    return request;
}

}  // namespace

TEST(S3ApiSignatureV4, AuthGetObject) {
    auto request = MakeRequest(clients::http::HttpMethod::kGet, "test.txt");
    request.headers[USERVER_NAMESPACE::http::headers::kRange] = "bytes=0-9";

    const auto headers = MakeAuthenticator().Auth(request);

    EXPECT_EQ(headers.at("X-Amz-Date"), "20130524T000000Z");
    EXPECT_EQ(headers.at("X-Amz-Content-Sha256"), kEmptyPayloadHash);
    EXPECT_EQ(
        headers.at("Authorization"),
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date, "
        "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41"
    );
}

TEST(S3ApiSignatureV4, AuthPutObject) {
    auto request = MakeRequest(clients::http::HttpMethod::kPut, "test$file.text");
    request.body = "Welcome to Amazon S3.";
    request.headers[USERVER_NAMESPACE::http::headers::kDate] = "Fri, 24 May 2013 00:00:00 GMT";
    request.headers[std::string_view{"x-amz-storage-class"}] = "REDUCED_REDUNDANCY";

    const auto headers = MakeAuthenticator().Auth(request);

    EXPECT_EQ(headers.at("X-Amz-Content-Sha256"), "44ce7dd67c959e0d3524ffac1771dfbba87d2b6b4b4e99e42034a8b803f8b072");
    EXPECT_EQ(
        headers.at("Authorization"),
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=date;host;x-amz-content-sha256;x-amz-date;x-amz-storage-class, "
        "Signature=98ad721746da40c64f1a55b78f14c238d841ea1380cd77a1b5971af0ece108bd"
    );
}

TEST(S3ApiSignatureV4, AuthQueryParameterWithoutValue) {
    const auto request = MakeRequest(clients::http::HttpMethod::kGet, "?lifecycle");

    const auto headers = MakeAuthenticator().Auth(request);

    EXPECT_EQ(
        headers.at("Authorization"),
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=fea454ca298b7da1c68078a5d1bdbfbbe0d65c699e0f91ac7a200a0136783543"
    );
}

TEST(S3ApiSignatureV4, AuthQueryParametersSorted) {
    const auto request = MakeRequest(clients::http::HttpMethod::kGet, "?prefix=J&max-keys=2");

    const auto headers = MakeAuthenticator().Auth(request);

    EXPECT_EQ(
        headers.at("Authorization"),
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=34b48302e7b5fa45bde8084f4b7868a86f0a534bc59db6670ed5711ef69dc6f7"
    );
}

TEST(S3ApiSignatureV4, AuthPathStyleAddressing) {
    auto request = MakeRequest(clients::http::HttpMethod::kGet, "test.txt");
    // the bucket is not a subdomain, so it becomes a part of the canonical URI
    request.headers[USERVER_NAMESPACE::http::headers::kHost] = "s3.amazonaws.com";

    const auto headers = MakeAuthenticator().Auth(request);

    EXPECT_EQ(
        headers.at("Authorization"),
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=0fcb291c4b47980ad34dd9a29532ceae67b48e45de3d6054873b430740567ec2"
    );
}

TEST(S3ApiSignatureV4, AuthEncodedPathAndQuery) {
    // the path is encoded the same way api_methods do: http::EncodeS3Key + http::MakeQuery
    const auto request = MakeRequest(clients::http::HttpMethod::kGet, "my%20folder/my%20file.txt?versionId=abc%20123");

    const auto headers = MakeAuthenticator().Auth(request);

    EXPECT_EQ(
        headers.at("Authorization"),
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=d6e900c34937a984ac3eeff28c985f58b4cbfa4ecfdb216af808afdef4145933"
    );
}

TEST(S3ApiSignatureV4, AuthRequiresHostHeader) {
    utils::datetime::MockNowSet(std::chrono::system_clock::from_time_t(kMockedNowEpoch));

    Request request;
    request.method = clients::http::HttpMethod::kGet;
    request.bucket = kBucket;
    request.req = "test.txt";

    EXPECT_THROW(MakeAuthenticator().Auth(request), std::runtime_error);
    EXPECT_THROW(MakeAuthenticator().Sign(request, kMockedNowEpoch + 60), std::runtime_error);
}

TEST(S3ApiSignatureV4, SignPresignedUrl) {
    const auto request = MakeRequest(clients::http::HttpMethod::kGet, "test.txt");

    const auto params = MakeAuthenticator().Sign(request, kMockedNowEpoch + 86400);

    EXPECT_EQ(params.at("X-Amz-Algorithm"), "AWS4-HMAC-SHA256");
    EXPECT_EQ(params.at("X-Amz-Credential"), "AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request");
    EXPECT_EQ(params.at("X-Amz-Date"), "20130524T000000Z");
    EXPECT_EQ(params.at("X-Amz-Expires"), "86400");
    EXPECT_EQ(params.at("X-Amz-SignedHeaders"), "host");
    EXPECT_EQ(params.at("X-Amz-Signature"), "aeeed9bbccd4d02ee5c0109b86d86835f995330da4c265957d157751f604d404");
}

TEST(S3ApiSignatureV4, SignPresignedUploadUrlSignsContentType) {
    auto request = MakeRequest(clients::http::HttpMethod::kPut, "test.txt");
    request.headers[USERVER_NAMESPACE::http::headers::kContentLength] = "21";
    request.headers[USERVER_NAMESPACE::http::headers::kContentType] = "text/plain";

    const auto params = MakeAuthenticator().Sign(request, kMockedNowEpoch + 86400);

    EXPECT_EQ(params.at("X-Amz-SignedHeaders"), "content-type;host");
    EXPECT_EQ(params.at("X-Amz-Signature"), "653d5b202899172830dfe331b817de16c682b8b1e366de1b251ae709a20e20f9");
}

TEST(S3ApiSignatureV4, SignPresignedUrlSignsAllRequestHeaders) {
    auto request = MakeRequest(clients::http::HttpMethod::kPut, "test.txt");
    request.headers[USERVER_NAMESPACE::http::headers::kContentType] = "image/png";
    request.headers[USERVER_NAMESPACE::http::headers::kContentDisposition] = "attachment";
    request.headers[std::string_view{"X-Amz-Storage-Class"}] = "REDUCED_REDUNDANCY";
    request.headers[std::string_view{"x-amz-meta-author"}] = "  Jane   Doe ";

    const auto params = MakeAuthenticator().Sign(request, kMockedNowEpoch + 86400);

    EXPECT_EQ(
        params.at("X-Amz-SignedHeaders"),
        "content-disposition;content-type;host;x-amz-meta-author;x-amz-storage-class"
    );
    EXPECT_EQ(params.at("X-Amz-Signature"), "56428fdf94646cc87e3c8563b85cec6dc69dcedca025b84ed6c854540554713a");
}

TEST(S3ApiSignatureV4, SignPresignedUploadUrlWithoutContentType) {
    auto request = MakeRequest(clients::http::HttpMethod::kPut, "test.txt");
    request.headers[USERVER_NAMESPACE::http::headers::kContentLength] = "0";
    request.headers[USERVER_NAMESPACE::http::headers::kContentType] = "";

    const auto params = MakeAuthenticator().Sign(request, kMockedNowEpoch + 86400);

    EXPECT_EQ(params.at("X-Amz-SignedHeaders"), "host");
    EXPECT_EQ(params.at("X-Amz-Signature"), "f4db56459304dafaa603a99a23c6bea8821890259a65c18ff503a4a72a80efd9");
}

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END

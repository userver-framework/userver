#include <userver/crypto/aws.hpp>

#include <chrono>
#include <stdexcept>

#include <userver/http/common_headers.hpp>
#include <userver/utils/mock_now.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

// Test vectors from
// https://docs.aws.amazon.com/AmazonS3/latest/developerguide/sig-v4-header-based-auth.html
constexpr std::string_view kAccessKey = "AKIAIOSFODNN7EXAMPLE";
constexpr std::string_view kSecretKey = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";
constexpr std::string_view kRegion = "us-east-1";
constexpr std::string_view kService = "s3";
constexpr std::string_view kVirtualHost = "examplebucket.s3.amazonaws.com";

constexpr std::string_view kEmptyPayloadHash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

// Fri, 24 May 2013 00:00:00 GMT
constexpr time_t kMockedNowEpoch = 1369353600;

http::headers::HeaderMap MakeHeaders(std::string_view host = kVirtualHost) {
    utils::datetime::MockNowSet(std::chrono::system_clock::from_time_t(kMockedNowEpoch));

    http::headers::HeaderMap headers;
    headers[http::headers::kHost] = std::string{host};
    return headers;
}

crypto::aws::SignV4Request MakeSignRequest(
    std::string_view method,
    std::string_view canonical_uri,
    std::string_view canonical_query = {},
    std::string_view payload = {}
) {
    return crypto::aws::SignV4Request{
        .http_method = method,
        .canonical_uri = canonical_uri,
        .canonical_query = canonical_query,
        .payload = payload,
        .access_key = kAccessKey,
        .secret_key = kSecretKey,
        .region = kRegion,
        .service = kService,
    };
}

}  // namespace

TEST(CryptoAwsSignRequestV4, AuthGetObject) {
    auto headers = MakeHeaders();
    headers[http::headers::kRange] = "bytes=0-9";

    crypto::aws::SignRequestV4(headers, MakeSignRequest("GET", "/test.txt"));

    EXPECT_EQ(headers[crypto::aws::kAmzDate], "20130524T000000Z");
    EXPECT_EQ(headers[crypto::aws::kAmzContentSha256], kEmptyPayloadHash);
    EXPECT_EQ(
        headers[http::headers::kAuthorization],
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date, "
        "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41"
    );
}

TEST(CryptoAwsSignRequestV4, AuthPutObject) {
    auto headers = MakeHeaders();
    headers[http::headers::kDate] = "Fri, 24 May 2013 00:00:00 GMT";
    headers.insert_or_assign("x-amz-storage-class", "REDUCED_REDUNDANCY");

    crypto::aws::SignRequestV4(headers, MakeSignRequest("PUT", "/test%24file.text", {}, "Welcome to Amazon S3."));

    EXPECT_EQ(
        headers[crypto::aws::kAmzContentSha256],
        "44ce7dd67c959e0d3524ffac1771dfbba87d2b6b4b4e99e42034a8b803f8b072"
    );
    EXPECT_EQ(
        headers[http::headers::kAuthorization],
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=date;host;x-amz-content-sha256;x-amz-date;x-amz-storage-class, "
        "Signature=98ad721746da40c64f1a55b78f14c238d841ea1380cd77a1b5971af0ece108bd"
    );
}

TEST(CryptoAwsSignRequestV4, AuthQueryParameterWithoutValue) {
    auto headers = MakeHeaders();
    crypto::aws::SignRequestV4(headers, MakeSignRequest("GET", "/", "lifecycle="));

    EXPECT_EQ(
        headers[http::headers::kAuthorization],
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=fea454ca298b7da1c68078a5d1bdbfbbe0d65c699e0f91ac7a200a0136783543"
    );
}

TEST(CryptoAwsSignRequestV4, AuthQueryParametersSorted) {
    auto headers = MakeHeaders();
    crypto::aws::SignRequestV4(headers, MakeSignRequest("GET", "/", "max-keys=2&prefix=J"));

    EXPECT_EQ(
        headers[http::headers::kAuthorization],
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=34b48302e7b5fa45bde8084f4b7868a86f0a534bc59db6670ed5711ef69dc6f7"
    );
}

TEST(CryptoAwsSignRequestV4, AuthPathStyleAddressing) {
    auto headers = MakeHeaders("s3.amazonaws.com");

    crypto::aws::SignRequestV4(headers, MakeSignRequest("GET", "/examplebucket/test.txt"));

    EXPECT_EQ(
        headers[http::headers::kAuthorization],
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=0fcb291c4b47980ad34dd9a29532ceae67b48e45de3d6054873b430740567ec2"
    );
}

TEST(CryptoAwsSignRequestV4, AuthEncodedPathAndQuery) {
    auto headers = MakeHeaders();
    crypto::aws::SignRequestV4(headers, MakeSignRequest("GET", "/my%20folder/my%20file.txt", "versionId=abc%20123"));

    EXPECT_EQ(
        headers[http::headers::kAuthorization],
        "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request, "
        "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
        "Signature=d6e900c34937a984ac3eeff28c985f58b4cbfa4ecfdb216af808afdef4145933"
    );
}

TEST(CryptoAwsSignRequestV4, AuthRequiresHostHeader) {
    utils::datetime::MockNowSet(std::chrono::system_clock::from_time_t(kMockedNowEpoch));

    http::headers::HeaderMap headers;
    EXPECT_THROW(crypto::aws::SignRequestV4(headers, MakeSignRequest("GET", "/test.txt")), std::runtime_error);
}

USERVER_NAMESPACE_END

#include "s3api_methods.hpp"

#include <unordered_set>

#include <fmt/format.h>

#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::api_methods {

TEST(S3ApiMethods, CopyObject) {
    const std::string source_bucket = "source_bucket";
    const std::string source_key = "source_key";
    const std::string dest_bucket = "dest_bucket";
    const std::string dest_key = "dest_key";
    const std::string content_type = "application/json";

    const Request request = CopyObject(source_bucket, source_key, dest_bucket, dest_key, content_type);
    EXPECT_EQ(request.method, USERVER_NAMESPACE::clients::http::HttpMethod::kPut);
    EXPECT_EQ(request.req, dest_key);
    EXPECT_EQ(request.bucket, dest_bucket);
    EXPECT_TRUE(request.body.empty());
    const std::string* copy_source = USERVER_NAMESPACE::utils::FindOrNullptr(request.headers, headers::kAmzCopySource);
    ASSERT_NE(copy_source, nullptr);
    EXPECT_EQ(*copy_source, fmt::format("/{}/{}", source_bucket, source_key));
}

TEST(S3ApiMethods, GetObjectWithRange) {
    const std::string bucket = "bucket";
    const std::string path = "path";
    const std::optional<std::string_view> version = "version";
    Request request = GetObject(bucket, path, version);
    SetRange(request, "Range: bytes=0-100");
    const std::string expected_value = "Range: bytes=0-100";
    EXPECT_STREQ(
        USERVER_NAMESPACE::utils::FindOrNullptr(request.headers, USERVER_NAMESPACE::http::headers::kRange)->c_str(),
        expected_value.c_str()
    );
}

TEST(S3ApiMethods, CreateMultipartUpload) {
    const std::string content_type = "application/octet-stream";
    const std::string content_disposition = "attachment; filename=some_file.bin";
    {
        multipart_upload::CreateMultipartUploadRequest request;
        request.key = "some-key";
        request.content_type = "application/binary";
        request.content_disposition = "attachment; filename=\"some_file.bin\"";
        request.content_encoding = "gzip";
        request.content_language = "ru-RU";

        const auto result = CreateInternalApiRequest("bucket", request);

        EXPECT_EQ(result.bucket, "bucket");
        EXPECT_EQ(result.req, "some-key?uploads");
        EXPECT_EQ(result.method, clients::http::HttpMethod::kPost);
        EXPECT_TRUE(result.body.empty());

        const auto expected_headers = clients::http::Headers{
            {USERVER_NAMESPACE::http::headers::kContentType, "application/binary"},
            {USERVER_NAMESPACE::http::headers::kContentDisposition, "attachment; filename=\"some_file.bin\""},
            {USERVER_NAMESPACE::http::headers::kContentLanguage, "ru-RU"},
            {USERVER_NAMESPACE::http::headers::kContentEncoding, "gzip"}
        };
        EXPECT_EQ(expected_headers, result.headers);
    }
    {
        multipart_upload::CreateMultipartUploadRequest request;
        request.key = "some-key";

        const auto result = CreateInternalApiRequest("bucket", request);

        EXPECT_EQ(result.bucket, "bucket");
        EXPECT_EQ(result.req, "some-key?uploads");
        EXPECT_EQ(result.method, clients::http::HttpMethod::kPost);
        EXPECT_TRUE(result.body.empty());
        EXPECT_EQ(clients::http::Headers{}, result.headers);
    }
}

TEST(S3ApiMethods, UploadPart) {
    multipart_upload::UploadPartRequest request;
    request.key = "some/key.bin";
    request.upload_id = "some_id";
    request.part_number = 2;
    request.data = "some data";

    const auto result = CreateInternalApiRequest("some-bucket", request);
    EXPECT_EQ(result.bucket, "some-bucket");
    EXPECT_EQ(result.method, clients::http::HttpMethod::kPut);
    const auto requested_query_params = utils::text::SplitIntoStringViewVector(http::ExtractQueryView(result.req), "&");
    EXPECT_EQ(
        std::unordered_set<std::string_view>({"partNumber=2", "uploadId=some_id"}),
        std::unordered_set<std::string_view>(requested_query_params.begin(), requested_query_params.end())
    );
    EXPECT_EQ(result.req.substr(0, result.req.find('?')), "some/key.bin");

    const auto expected_headers = clients::http::Headers{
        {USERVER_NAMESPACE::http::headers::kContentType, "application/octet-stream"},
        {USERVER_NAMESPACE::http::headers::kContentLength, std::to_string(result.body.size())},
    };
    EXPECT_EQ(expected_headers, result.headers);
    EXPECT_EQ(result.body, request.data);
}

TEST(S3ApiMethods, AbortMultipartUpload) {
    multipart_upload::AbortMultipartUploadRequest request;
    request.key = "some/key.bin";
    request.upload_id = "some_id";

    const auto result = CreateInternalApiRequest("some-bucket", request);
    EXPECT_EQ(result.bucket, "some-bucket");
    EXPECT_EQ(result.method, clients::http::HttpMethod::kDelete);
    EXPECT_EQ(result.req, "some/key.bin?uploadId=some_id");
}

TEST(S3ApiMethods, CompleteMultipartUpload) {
    multipart_upload::CompleteMultipartUploadRequest request;
    request.key = "some-key";
    request.upload_id = "some_id";
    request.completed_parts = {{1, "\"etag1\""}, {2, "\"etag2\""}, {3, "\"etag3\""}};

    const auto result = CreateInternalApiRequest("some-bucket", request);

    EXPECT_EQ(result.bucket, "some-bucket");
    EXPECT_EQ(result.method, clients::http::HttpMethod::kPost);
    EXPECT_EQ(result.req, "some-key?uploadId=some_id");

    const auto expected_headers = clients::http::Headers{
        {USERVER_NAMESPACE::http::headers::kContentType, "application/xml; charset=UTF-8"},
        {USERVER_NAMESPACE::http::headers::kContentLength, std::to_string(result.body.size())},
    };
    EXPECT_EQ(expected_headers, result.headers);
    EXPECT_EQ(
        result.body,
        "<?xml version=\"1.0\"?><CompleteMultipartUpload xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
        "<Part><PartNumber>1</PartNumber><ETag>\"etag1\"</ETag></Part>"
        "<Part><PartNumber>2</PartNumber><ETag>\"etag2\"</ETag></Part>"
        "<Part><PartNumber>3</PartNumber><ETag>\"etag3\"</ETag></Part>"
        "</CompleteMultipartUpload>"
    );
}

TEST(S3ApiMethods, ListMultipartUploads) {
    {
        multipart_upload::ListMultipartUploadsRequest request;
        const auto result = CreateInternalApiRequest("some-bucket", request);
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.method, clients::http::HttpMethod::kGet);
        EXPECT_EQ(result.req, "?uploads");
        EXPECT_EQ(result.headers, clients::http::Headers{});
        EXPECT_TRUE(result.body.empty());
    }
    {
        multipart_upload::ListMultipartUploadsRequest request;
        request.prefix = "some/prefix/to";
        const auto result = CreateInternalApiRequest("some-bucket", request);
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.method, clients::http::HttpMethod::kGet);
        EXPECT_EQ(result.headers, clients::http::Headers{});
        EXPECT_TRUE(result.body.empty());

        const auto requested_query_params = utils::text::SplitIntoStringViewVector(result.req, "&");
        EXPECT_EQ(
            std::unordered_set<std::string_view>({"?uploads", "prefix=some%2Fprefix%2Fto"}),
            std::unordered_set<std::string_view>(requested_query_params.begin(), requested_query_params.end())
        );
    }
    {
        multipart_upload::ListMultipartUploadsRequest request;
        request.prefix = "some/prefix/to";
        request.max_uploads = 100;
        request.delimiter = "/";
        request.key_marker = "abc";
        request.upload_id_marker = "def";

        const auto result = CreateInternalApiRequest("some-bucket", request);
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.method, clients::http::HttpMethod::kGet);
        EXPECT_EQ(result.headers, clients::http::Headers{});
        EXPECT_TRUE(result.body.empty());

        const auto requested_query_params = utils::text::SplitIntoStringViewVector(result.req, "&");
        EXPECT_EQ(
            std::unordered_set<std::string_view>(
                {"?uploads",
                 "prefix=some%2Fprefix%2Fto",
                 "max-uploads=100",
                 "marker=abc",
                 "delimiter=%2F",
                 "upload-id-marker=def"}
            ),
            std::unordered_set<std::string_view>(requested_query_params.begin(), requested_query_params.end())
        );
    }
}

TEST(S3ApiMethods, ListParts) {
    {
        multipart_upload::ListPartsRequest request;
        request.key = "some/path/to";
        request.upload_id = "some_id";
        const auto result = CreateInternalApiRequest("some-bucket", request);
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.method, clients::http::HttpMethod::kGet);
        EXPECT_EQ(result.req, "some/path/to?uploadId=some_id");
        EXPECT_EQ(result.headers, clients::http::Headers{});
    }
    {
        multipart_upload::ListPartsRequest request;
        request.key = "some/path/to";
        request.upload_id = "some_id";
        request.max_parts = 10;
        request.part_number_marker = 11;

        const auto result = CreateInternalApiRequest("some-bucket", request);
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.method, clients::http::HttpMethod::kGet);

        const auto
            requested_query_params = utils::text::SplitIntoStringViewVector(http::ExtractQueryView(result.req), "&");
        EXPECT_EQ(
            std::unordered_set<std::string_view>({"uploadId=some_id", "max-parts=10", "part-number-marker=11"}),
            std::unordered_set<std::string_view>(requested_query_params.begin(), requested_query_params.end())
        );
        EXPECT_EQ(result.req.substr(0, result.req.find('?')), "some/path/to");

        EXPECT_EQ(result.headers, clients::http::Headers{});
    }
}

}  // namespace s3api::api_methods

USERVER_NAMESPACE_END

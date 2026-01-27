#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::multipart_upload {

/// For some unclear reasons almost all fields in S3 API are optional
/// including even those fields which are clearly stated to be required
/// in S3 documentation.
/// There are some unconvincing explanations for this at
///  https://github.com/boto/botocore/issues/1069#issuecomment-259255047, but that's it!
/// This implementation of the API in this library is a bit stricter than it is in S3 API
/// specifciation, intentionally making some `optional` fields to be `required non-empty`.

/// The response body content of CreateMultipartUpload action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_CreateMultipartUpload.html#API_CreateMultipartUpload_ResponseSyntax
/// NOTE: `bucket`, `key`, `upload_id` are allowed to be empty string values by the S3 specification.
struct InitiateMultipartUploadResult {
    std::string bucket;
    std::string key;
    std::string upload_id;

    static InitiateMultipartUploadResult Parse(utils::zstring_view http_s3_respose_body);
};

/// The response body content of CompleteMultipartUpload action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_CompleteMultipartUpload.html#API_CompleteMultipartUpload_ResponseSyntax
/// May also include ETag, Checksum*, etc.
/// NOTE: `bucket`, `key`, `location` are allowed to be empty string values by the S3 specification.
struct CompleteMultipartUploadResult {
    std::string bucket;
    std::string key;
    std::string location;

    static CompleteMultipartUploadResult Parse(utils::zstring_view http_s3_respose_body);
};

/// The response body content of CompleteMultipartUpload action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListMultipartUploads.html#API_ListMultipartUploads_ResponseSyntax
/// NOTE:
/// Do not use the result of this listing when sending a complete multipart upload request.
/// Instead, maintain your own list of the part numbers that you specified when uploading parts
/// and the corresponding ETag values that Amazon S3 returns.
/// (details in https://docs.aws.amazon.com/AmazonS3/latest/userguide/mpuoverview.html)
struct ListMultipartUploadsResult {
    struct MultipartUpload {
        std::string key;
        std::string upload_id;
        std::optional<std::chrono::system_clock::time_point> initiated_ts;
    };

    std::string bucket;
    std::optional<std::string> key_marker;
    std::optional<std::string> upload_id_marker;
    std::optional<std::string> next_key_marker;
    std::optional<std::string> next_upload_id_marker;
    std::optional<std::string> delimiter;
    bool is_truncated{false};
    std::optional<unsigned> max_uploads;
    std::vector<MultipartUpload> uploads;
    std::vector<std::string> common_prefixes;

    static ListMultipartUploadsResult Parse(utils::zstring_view http_s3_respose_body);
};

/// The response body content of ListParts action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListParts.html#API_ListParts_ResponseSyntax
/// May also include Initiator, Owner, Checksum*, StorageClass and etc.
struct ListPartsResult {
    struct Part {
        std::string etag;
        unsigned part_number;
        std::optional<std::chrono::system_clock::time_point> last_modified_ts;
        std::optional<unsigned long> byte_size;
    };

    std::string bucket;
    std::string key;
    std::string upload_id;
    std::optional<unsigned> max_parts;
    std::optional<unsigned> part_number_marker;
    std::optional<unsigned> next_part_number_marker;
    bool is_truncated{false};
    std::vector<Part> parts;

    static ListPartsResult Parse(utils::zstring_view http_s3_respose_body);
};

/// The response struct containing response header values for UploadPart action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_UploadPart.html#API_UploadPart_ResponseSyntax
/// May also include checksum headers and etc.
struct UploadPartResult {
    std::string etag;
};

}  // namespace s3api::multipart_upload

USERVER_NAMESPACE_END

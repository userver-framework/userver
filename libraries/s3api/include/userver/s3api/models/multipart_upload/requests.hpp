#pragma once

#include <optional>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace s3api {

struct Request;

namespace multipart_upload {

/// For some unclear reasons almost all fields in S3 API are optional
/// including even those fields which are clearly stated to be required
/// in S3 documentation.
/// There are some unconvincing explanations for this at
///  https://github.com/boto/botocore/issues/1069#issuecomment-259255047, but that's it!
/// This implementation of the API in this library is a bit stricter than it is in S3 API
/// specifciation, intentionally making some `optional` fields to be `required non-empty`.

/// CreateMultipartUpload action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_CreateMultipartUpload.html#API_CreateMultipartUpload_RequestSyntax
/// NOTE:
///     - `key` is expected to be URL encoded
struct CreateMultipartUploadRequest {
    std::string key;
    std::optional<std::string> content_type;
    std::optional<std::string> content_encoding;
    std::optional<std::string> content_disposition;
    std::optional<std::string> content_language;
};

/// CompleteMultipartUpload action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_CreateMultipartUpload.html#API_CreateMultipartUpload_RequestSyntax
/// NOTE:
///   - For each part in the list of completed parts, you must provide the
///     PartNumber value and the ETag value that are returned after that part was uploaded.
///     All parts must be placed in ascending order by their PartNumber.
///     @see https://docs.aws.amazon.com/AmazonS3/latest/API/API_CompleteMultipartUpload.html
///   - `key` is expected to be URL encoded;
struct CompleteMultipartUploadRequest {
    /// @see https://docs.aws.amazon.com/AmazonS3/latest/API/API_CompletedPart.html
    // It may also contain optional Checksum* attributes.
    // The specification states that all attributes are optional, however
    // CompleteMultipartUpload action specification explicitly requires ETag and PartNumber.
    struct CompletedPart {
        unsigned part_number;
        std::string etag;
    };

    std::string key;
    std::string upload_id;
    std::vector<CompletedPart> completed_parts;
};

/// AbortMultipartUpload action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_AbortMultipartUpload.html#API_AbortMultipartUpload_RequestSyntax
/// NOTE:
///     - `key` is expected to be URL encoded
struct AbortMultipartUploadRequest {
    std::string key;
    std::string upload_id;
};

/// UploadPart action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_UploadPart.html#API_UploadPart_RequestSyntax
struct UploadPartRequest {
    std::string key;
    std::string upload_id;
    unsigned part_number{0};
    std::string data;
};

/// ListParts action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListParts.html#API_ListParts_RequestSyntax
/// NOTE:
///     - The ListParts request returns a maximum of 1,000 uploaded parts.
///       The limit of 1,000 parts is also the default value.
///     - `key` is expected to be URL encoded
struct ListPartsRequest {
    std::string key;
    std::string upload_id;
    unsigned max_parts{1000u};
    std::optional<unsigned> part_number_marker;
};

/// ListMultipartUploads action request
/// @see
/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListMultipartUploads.html#API_ListMultipartUploads_RequestSyntax
struct ListMultipartUploadsRequest {
    enum class EncodingType { kNone, kUrl };

    EncodingType encoding_type{EncodingType::kNone};
    unsigned max_uploads{1000u};
    std::optional<std::string> delimiter;
    std::optional<std::string> key_marker;
    std::optional<std::string> prefix;
    std::optional<std::string> upload_id_marker;
};

}  // namespace multipart_upload
}  // namespace s3api

USERVER_NAMESPACE_END

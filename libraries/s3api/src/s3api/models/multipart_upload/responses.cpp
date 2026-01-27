#include <userver/s3api/models/multipart_upload/responses.hpp>

#include <fmt/format.h>
#include <pugixml.hpp>

#include <userver/s3api/models/errors.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::multipart_upload {
namespace {

pugi::xml_node GetRequiredChildNode(const pugi::xml_document& doc, const char* child_name) {
    auto child = doc.child(child_name);
    if (!child) {
        throw ResponseParsingError(fmt::format("document is missing root child node '{}'", child_name));
    }
    return child;
}

std::optional<std::string_view> ExtractChildValue(
    const pugi::xml_node& node,
    const char* child_name,
    bool is_empty_allowed = true
) {
    const auto child = node.child(child_name);
    auto result = (!child.empty() ? std::make_optional<std::string_view>(child.child_value()) : std::nullopt);
    if (!is_empty_allowed && result && result->empty()) {
        throw ResponseParsingError(fmt::format("node '{}.{}' is not expected to be empty", node.name(), child_name));
    }
    return result;
}

std::string_view ExtractRequiredChildValue(const pugi::xml_node& node, const char* child_name) {
    const auto result = ExtractChildValue(node, child_name, false);
    if (!result) {
        throw ResponseParsingError(
            fmt::format("node '{}' is missing the required child node '{}'", node.name(), child_name)
        );
    }
    return *result;
}

bool ToBoolean(const std::optional<std::string_view>& maybe_str) {
    if (maybe_str.value_or("false") != "true") {
        return false;
    }
    return true;
}

template <typename T>
std::enable_if_t<std::is_integral_v<T>, std::optional<T>> ExtractChildValueAsIntegral(
    const pugi::xml_node& node,
    const char* child_name
) try
{
    const auto maybe_str = ExtractChildValue(node, child_name, false);
    if (!maybe_str) {
        return std::nullopt;
    }

    return USERVER_NAMESPACE::utils::FromString<T>(*maybe_str);

} catch (const std::exception& exc) {
    throw ResponseParsingError(
        fmt::format("failed to parse node '{}.{}' value as integral type - {}", node.name(), child_name, exc.what())
    );
}

template <typename T>
std::enable_if_t<std::is_integral_v<T>, T> ExtractRequiredChildValueAsIntegral(
    const pugi::xml_node& node,
    const char* child_name
) {
    const auto maybe_value = ExtractChildValueAsIntegral<T>(node, child_name);
    if (!maybe_value) {
        throw ResponseParsingError(
            fmt::format("node '{}' is missing the required child node '{}'", node.name(), child_name)
        );
    }
    return *maybe_value;
}

constexpr auto kExtractRequiredChildValueAsUInt = ExtractRequiredChildValueAsIntegral<unsigned>;
constexpr auto kExtractChildValueAsUInt = ExtractChildValueAsIntegral<unsigned>;
constexpr auto kExtractChildValueAsULong = ExtractChildValueAsIntegral<unsigned long>;

}  // namespace

InitiateMultipartUploadResult InitiateMultipartUploadResult::Parse(utils::zstring_view http_s3_respose_body) {
    InitiateMultipartUploadResult result;
    pugi::xml_document xml;
    const pugi::xml_parse_result
        parse_result = xml.load_string(http_s3_respose_body.c_str(), pugi::parse_default | pugi::parse_escapes);
    if (parse_result.status != pugi::status_ok) {
        throw ResponseParsingError(parse_result.description());
    }
    const auto xml_root_item = GetRequiredChildNode(xml, "InitiateMultipartUploadResult");
    result.bucket = ExtractRequiredChildValue(xml_root_item, "Bucket");
    result.upload_id = ExtractRequiredChildValue(xml_root_item, "UploadId");
    result.key = ExtractRequiredChildValue(xml_root_item, "Key");
    return result;
}

CompleteMultipartUploadResult CompleteMultipartUploadResult::Parse(utils::zstring_view http_s3_respose_body) {
    CompleteMultipartUploadResult result;
    pugi::xml_document xml;
    const pugi::xml_parse_result
        parse_result = xml.load_string(http_s3_respose_body.c_str(), pugi::parse_default | pugi::parse_escapes);
    if (parse_result.status != pugi::status_ok) {
        throw ResponseParsingError(parse_result.description());
    }
    const auto xml_root_item = GetRequiredChildNode(xml, "CompleteMultipartUploadResult");
    result.location = ExtractRequiredChildValue(xml_root_item, "Location");
    result.bucket = ExtractRequiredChildValue(xml_root_item, "Bucket");
    result.key = ExtractRequiredChildValue(xml_root_item, "Key");
    return result;
}

ListMultipartUploadsResult ListMultipartUploadsResult::Parse(utils::zstring_view http_s3_respose_body) {
    ListMultipartUploadsResult result;
    pugi::xml_document xml;
    const pugi::xml_parse_result
        parse_result = xml.load_string(http_s3_respose_body.c_str(), pugi::parse_default | pugi::parse_escapes);
    if (parse_result.status != pugi::status_ok) {
        throw ResponseParsingError(parse_result.description());
    }

    const auto xml_root_item = GetRequiredChildNode(xml, "ListMultipartUploadsResult");
    result.bucket = ExtractRequiredChildValue(xml_root_item, "Bucket");
    result.key_marker = ExtractChildValue(xml_root_item, "KeyMarker");
    result.upload_id_marker = ExtractChildValue(xml_root_item, "UploadIdMarker");
    result.next_key_marker = ExtractChildValue(xml_root_item, "NextKeyMarker");
    result.next_upload_id_marker = ExtractChildValue(xml_root_item, "NextUploadIdMarker");
    result.delimiter = ExtractChildValue(xml_root_item, "Delimiter");
    result.is_truncated = ToBoolean(ExtractChildValue(xml_root_item, "IsTruncated"));
    result.max_uploads = kExtractChildValueAsUInt(xml_root_item, "MaxUploads");

    for (auto node = xml_root_item.child("Upload"); node; node = node.next_sibling("Upload")) {
        ListMultipartUploadsResult::MultipartUpload multipart_upload;
        multipart_upload.key = ExtractRequiredChildValue(node, "Key");
        multipart_upload.upload_id = ExtractRequiredChildValue(node, "UploadId");
        if (const auto maybe_initiated_str = ExtractChildValue(node, "Initiated")) {
            // See S3 client aws-sdk-cpp implementaton references:
            // https://github.com/aws/aws-sdk-cpp/blob/6762e8220ae37a35c7a8f762f9b97c5d2eda7455/generated/src/aws-cpp-sdk-s3/source/model/MultipartUpload.cpp#L49
            multipart_upload.initiated_ts =
                USERVER_NAMESPACE::utils::datetime::GuessStringtime(std::string{*maybe_initiated_str}, "UTC");
        }
        result.uploads.push_back(std::move(multipart_upload));
    }

    for (auto node = xml_root_item.child("CommonPrefixes"); node; node = node.next_sibling("CommonPrefixes")) {
        result.common_prefixes.emplace_back(ExtractRequiredChildValue(node, "Prefix"));
    }

    return result;
}

ListPartsResult ListPartsResult::Parse(utils::zstring_view http_s3_respose_body) {
    ListPartsResult result;
    pugi::xml_document xml;
    const pugi::xml_parse_result
        parse_result = xml.load_string(http_s3_respose_body.c_str(), pugi::parse_default | pugi::parse_escapes);
    if (parse_result.status != pugi::status_ok) {
        throw ResponseParsingError(parse_result.description());
    }

    const auto xml_root_item = GetRequiredChildNode(xml, "ListPartsResult");
    result.bucket = ExtractRequiredChildValue(xml_root_item, "Bucket");
    result.upload_id = ExtractRequiredChildValue(xml_root_item, "UploadId");
    result.max_parts = kExtractChildValueAsUInt(xml_root_item, "MaxParts");
    result.part_number_marker = kExtractChildValueAsUInt(xml_root_item, "PartNumberMarker");
    result.next_part_number_marker = kExtractChildValueAsUInt(xml_root_item, "NextPartNumberMarker");
    result.key = ExtractRequiredChildValue(xml_root_item, "Key");
    result.is_truncated = ToBoolean(ExtractChildValue(xml_root_item, "IsTruncated"));

    for (auto part_node = xml_root_item.child("Part"); part_node; part_node = part_node.next_sibling("Part")) {
        ListPartsResult::Part part;
        part.etag = ExtractRequiredChildValue(part_node, "ETag");
        part.part_number = kExtractRequiredChildValueAsUInt(part_node, "PartNumber");
        part.byte_size = kExtractChildValueAsULong(part_node, "Size");

        if (const auto maybe_last_modified_ts_str = ExtractChildValue(part_node, "LastModified")) {
            // See S3 client aws-sdk-cpp implementaton references:
            // https://github.com/aws/aws-sdk-cpp/blob/6762e8220ae37a35c7a8f762f9b97c5d2eda7455/generated/src/aws-cpp-sdk-s3/source/model/MultipartUpload.cpp#L49
            part.last_modified_ts =
                USERVER_NAMESPACE::utils::datetime::GuessStringtime(std::string{*maybe_last_modified_ts_str}, "UTC");
        }

        result.parts.push_back(std::move(part));
    }

    return result;
}

}  // namespace s3api::multipart_upload

USERVER_NAMESPACE_END

#include <userver/s3api/models/multipart_upload/responses.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::multipart_upload {

TEST(S3ApiInitiateMultipartUploadResult, ParseResponseBody) {
    const auto result = InitiateMultipartUploadResult::Parse(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<InitiateMultipartUploadResult>"
        "<Bucket>some-bucket</Bucket><Key>some/key</Key><UploadId>123456ABCDEF</UploadId>"
        "</InitiateMultipartUploadResult>"
    );
    EXPECT_EQ(result.bucket, "some-bucket");
    EXPECT_EQ(result.key, "some/key");
    EXPECT_EQ(result.upload_id, "123456ABCDEF");
}

TEST(S3ApiCompleteMultipartUploadeResult, ParseResponseBody) {
    const auto result = CompleteMultipartUploadResult::Parse(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<CompleteMultipartUploadResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
        "<Location>https://some-bucket.some-s3.net/some/key</Location>"
        "<Bucket>some-bucket</Bucket><Key>some/key</Key>"
        "<UploadId>123456ABCDEF</UploadId>"
        "<ETag>\"123456\"</ETag>"
        "</CompleteMultipartUploadResult>"
    );
    EXPECT_EQ(result.bucket, "some-bucket");
    EXPECT_EQ(result.key, "some/key");
    EXPECT_EQ(result.location, "https://some-bucket.some-s3.net/some/key");
}

TEST(S3ApiListMultipartUploadsResult, ParseResponseBody) {
    using std::chrono::milliseconds;
    using TimePoint = std::chrono::system_clock::time_point;

    // no multipart uploads
    {
        const auto result = ListMultipartUploadsResult::Parse(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<ListMultipartUploadsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            "<Bucket>some-bucket</Bucket>"
            "<KeyMarker></KeyMarker><UploadIdMarker/><NextKeyMarker/><NextUploadIdMarker/>"
            "<IsTruncated>false</IsTruncated>"
            "<MaxUploads>1000</MaxUploads>"
            "</ListMultipartUploadsResult>"
        );
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.key_marker, std::string());
        EXPECT_EQ(result.upload_id_marker, std::string());
        EXPECT_EQ(result.next_key_marker, std::string());
        EXPECT_EQ(result.next_upload_id_marker, std::string());
        EXPECT_FALSE(result.is_truncated);
        EXPECT_EQ(result.max_uploads, 1000);
        EXPECT_TRUE(result.uploads.empty());
        EXPECT_TRUE(result.common_prefixes.empty());
    }
    // 2 active multipart uploads
    {
        const auto result = ListMultipartUploadsResult::Parse(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<ListMultipartUploadsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            "<Bucket>some-bucket</Bucket>"
            "<KeyMarker/><UploadIdMarker/><NextKeyMarker/><NextUploadIdMarker/>"
            "<IsTruncated>false</IsTruncated>"
            "<MaxUploads>10</MaxUploads>"
            "<Upload>"
            "<Initiated>2025-08-29T09:29:46.506Z</Initiated><Key>a1/b1/c1</Key>"
            "<StorageClass>STANDARD</StorageClass><UploadId>00063D7DA9A30F29</UploadId>"
            "</Upload><Upload>"
            "<Initiated>2025-08-29T09:29:46Z</Initiated><Key>a1/b1/c2</Key>"
            "<StorageClass>STANDARD</StorageClass><UploadId>00063D7DA9A77808</UploadId>"
            "</Upload></ListMultipartUploadsResult>"
        );
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.key_marker, std::string());
        EXPECT_EQ(result.upload_id_marker, std::string());
        EXPECT_EQ(result.next_key_marker, std::string());
        EXPECT_EQ(result.next_upload_id_marker, std::string());
        EXPECT_TRUE(!result.delimiter);
        EXPECT_FALSE(result.is_truncated);
        EXPECT_EQ(result.max_uploads, 10);
        ASSERT_EQ(result.uploads.size(), 2);
        EXPECT_EQ(result.uploads[0].key, "a1/b1/c1");
        EXPECT_EQ(result.uploads[0].upload_id, "00063D7DA9A30F29");
        EXPECT_EQ(result.uploads[0].initiated_ts, TimePoint(milliseconds(1756459786506)));
        EXPECT_EQ(result.uploads[1].key, "a1/b1/c2");
        EXPECT_EQ(result.uploads[1].upload_id, "00063D7DA9A77808");
        EXPECT_EQ(result.uploads[1].initiated_ts, TimePoint(milliseconds(1756459786000)));
        EXPECT_TRUE(result.common_prefixes.empty());
    }
    // with one delimiter, with max uploads 2
    {
        const auto result = ListMultipartUploadsResult::Parse(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<ListMultipartUploadsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            "<Bucket>some-bucket</Bucket><KeyMarker>a_1</KeyMarker>"
            "<UploadIdMarker>00063D7E07565264</UploadIdMarker>"
            "<Delimiter>/</Delimiter><IsTruncated>true</IsTruncated>"
            "<NextKeyMarker>a_4</NextKeyMarker>"
            "<NextUploadIdMarker>00063D7E075A93B9</NextUploadIdMarker>"
            "<MaxUploads>2</MaxUploads>"
            "<Upload>"
            "<Initiated>2025-08-29T09:55:58.535Z</Initiated><Key>a_3</Key>"
            "<StorageClass>STANDARD</StorageClass><UploadId>00063D7E07565265</UploadId>"
            "</Upload><Upload><Key>a_4</Key>"
            "<StorageClass>STANDARD</StorageClass><UploadId>00063D7E075A93B9</UploadId>"
            "</Upload>"
            "<CommonPrefixes><Prefix>a_1/</Prefix></CommonPrefixes>"
            "<CommonPrefixes><Prefix>a_2/</Prefix></CommonPrefixes>"
            "</ListMultipartUploadsResult>"
        );
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.key_marker, "a_1");
        EXPECT_EQ(result.upload_id_marker, "00063D7E07565264");
        EXPECT_EQ(result.next_key_marker, "a_4");
        EXPECT_EQ(result.next_upload_id_marker, "00063D7E075A93B9");
        EXPECT_EQ(result.delimiter, "/");
        EXPECT_TRUE(result.is_truncated);
        EXPECT_EQ(result.max_uploads, 2);
        ASSERT_EQ(result.uploads.size(), 2);
        EXPECT_EQ(result.uploads[0].key, "a_3");
        EXPECT_EQ(result.uploads[0].upload_id, "00063D7E07565265");
        EXPECT_EQ(result.uploads[0].initiated_ts, TimePoint(milliseconds(1756461358535)));
        EXPECT_EQ(result.uploads[1].key, "a_4");
        EXPECT_EQ(result.uploads[1].upload_id, "00063D7E075A93B9");
        ASSERT_FALSE(result.uploads[1].initiated_ts.has_value());
        ASSERT_EQ(result.common_prefixes.size(), 2);
        EXPECT_EQ(result.common_prefixes[0], "a_1/");
        EXPECT_EQ(result.common_prefixes[1], "a_2/");
    }
}

TEST(S3ApiListPartsResult, ParseResponseBody) {
    using std::chrono::milliseconds;
    using TimePoint = std::chrono::system_clock::time_point;

    // no parts in multipart upload
    {
        const auto result = ListPartsResult::Parse(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<ListPartsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            "<Bucket>some-bucket</Bucket>"
            "<Key>a_1/b_1/c_1</Key><UploadId>00063DD078D0EFFC</UploadId>"
            "<StorageClass>STANDARD</StorageClass><PartNumberMarker>0</PartNumberMarker>"
            "<MaxParts>1000</MaxParts>"
            "<IsTruncated>false</IsTruncated>"
            "</ListPartsResult>"
        );
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.key, "a_1/b_1/c_1");
        EXPECT_EQ(result.upload_id, "00063DD078D0EFFC");
        EXPECT_EQ(result.max_parts, 1000);
        EXPECT_EQ(result.part_number_marker, 0);
        EXPECT_FALSE(result.next_part_number_marker.has_value());
        EXPECT_FALSE(result.is_truncated);
        EXPECT_TRUE(result.parts.empty());
    }
    // 3 parts; not truncated
    {
        const auto result = ListPartsResult::Parse(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<ListPartsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            "<Bucket>some-bucket</Bucket>"
            "<Key>a_1/b_1/c_1</Key><UploadId>00063DD078D0EFFC</UploadId>"
            "<StorageClass>STANDARD</StorageClass><PartNumberMarker>0</PartNumberMarker>"
            "<MaxParts>10</MaxParts>"
            "<IsTruncated>false</IsTruncated>"
            "<Part><PartNumber>1</PartNumber>"
            "<LastModified>2025-09-02T12:15:47.363Z</LastModified>"
            "<ETag>&#34;975cd7fe7fd02f5556d0410161f9243e&#34;</ETag>"
            "<Size>104857600</Size></Part>"
            "<Part><PartNumber>2</PartNumber>"
            "<LastModified>2025-09-02T12:15:50.482Z</LastModified>"
            "<ETag>&#34;865db48e2a10f7fc8e2d06a5cc390f17&#34;</ETag>"
            "<Size>104857600</Size></Part>"
            "<Part><PartNumber>3</PartNumber>"
            "<LastModified>2025-09-02T12:15:53.717Z</LastModified>"
            "<ETag>&#34;44f2b11f5bf7f61283eab82bbe0998ca&#34;</ETag>"
            "<Size>104857600</Size></Part>"
            "</ListPartsResult>"
        );
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.key, "a_1/b_1/c_1");
        EXPECT_EQ(result.upload_id, "00063DD078D0EFFC");
        EXPECT_EQ(result.max_parts, 10);
        EXPECT_EQ(result.part_number_marker, 0);
        EXPECT_FALSE(result.next_part_number_marker.has_value());
        EXPECT_FALSE(result.is_truncated);
        ASSERT_EQ(result.parts.size(), 3u);
        EXPECT_EQ(result.parts[0].part_number, 1);
        EXPECT_EQ(result.parts[0].byte_size, 104857600);
        EXPECT_EQ(result.parts[0].last_modified_ts, TimePoint(milliseconds(1756815347363)));
        EXPECT_EQ(result.parts[0].etag, "\"975cd7fe7fd02f5556d0410161f9243e\"");
        EXPECT_EQ(result.parts[1].part_number, 2);
        EXPECT_EQ(result.parts[1].byte_size, 104857600);
        EXPECT_EQ(result.parts[1].last_modified_ts, TimePoint(milliseconds(1756815350482)));
        EXPECT_EQ(result.parts[1].etag, "\"865db48e2a10f7fc8e2d06a5cc390f17\"");
        EXPECT_EQ(result.parts[2].part_number, 3);
        EXPECT_EQ(result.parts[2].byte_size, 104857600);
        EXPECT_EQ(result.parts[2].last_modified_ts, TimePoint(milliseconds(1756815353717)));
        EXPECT_EQ(result.parts[2].etag, "\"44f2b11f5bf7f61283eab82bbe0998ca\"");
    }
    // 1 part and truncated
    {
        const auto result = ListPartsResult::Parse(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<ListPartsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            "<Bucket>some-bucket</Bucket>"
            "<Key>a_1/b_1/c_1</Key><UploadId>00063DD078D0EFFC</UploadId>"
            "<StorageClass>STANDARD</StorageClass><PartNumberMarker>1</PartNumberMarker>"
            "<NextPartNumberMarker>2</NextPartNumberMarker><MaxParts>1</MaxParts>"
            "<IsTruncated>true</IsTruncated>"
            "<Part><PartNumber>2</PartNumber>"
            "<LastModified>2025-09-02T12:15:50.482Z</LastModified>"
            "<ETag>&#34;865db48e2a10f7fc8e2d06a5cc390f17&#34;</ETag>"
            "<Size>104857600</Size></Part>"
            "</ListPartsResult>"
        );
        EXPECT_EQ(result.bucket, "some-bucket");
        EXPECT_EQ(result.key, "a_1/b_1/c_1");
        EXPECT_EQ(result.upload_id, "00063DD078D0EFFC");
        EXPECT_EQ(result.max_parts, 1);
        EXPECT_EQ(result.part_number_marker, 1);
        EXPECT_EQ(result.next_part_number_marker, 2);
        EXPECT_TRUE(result.is_truncated);
        ASSERT_EQ(result.parts.size(), 1u);
        EXPECT_EQ(result.parts[0].part_number, 2);
        EXPECT_EQ(result.parts[0].byte_size, 104857600);
        EXPECT_EQ(result.parts[0].last_modified_ts, TimePoint(milliseconds(1756815350482)));
        EXPECT_EQ(result.parts[0].etag, "\"865db48e2a10f7fc8e2d06a5cc390f17\"");
    }
}

}  // namespace s3api::multipart_upload

USERVER_NAMESPACE_END

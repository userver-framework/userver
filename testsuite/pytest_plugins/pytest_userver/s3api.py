import collections
from collections.abc import Mapping
from collections.abc import MutableMapping
import dataclasses
import datetime as dt
import hashlib
import os
import pathlib
import sys
import xml.etree.ElementTree

import dateutil.tz as tz


class _S3NoSuchUploadError(Exception):
    code = 'NoSuchUpload'
    message = 'The specified multipart upload does not exist.'

    def __str__(self):
        return _S3NoSuchUploadError.message


class _S3InvalidPartError(Exception):
    code = 'InvalidPart'
    message = (
        'One or more of the specified parts could not be found.'
        ' The part might not have been uploaded, or the specified'
        " ETag might not have matched the uploaded part's ETag."
    )

    def __str__(self):
        return _S3InvalidPartError.message


class _S3InvalidPartOrderError(Exception):
    code = 'InvalidPartOrder'
    message = 'The list of parts was not in ascending order. The parts list must be specified in order by part number.'

    def __str__(self):
        return _S3InvalidPartOrderError.message


class _S3EntityTooSmallError(Exception):
    code = 'EntityTooSmall'
    message = 'Your proposed upload is smaller than the minimum allowed object size.'

    def __str__(self):
        return _S3EntityTooSmallError.message


class _S3ClientError(Exception):
    def __init__(self, msg: str):
        self._msg = msg

    def __str__(self):
        return self._msg


@dataclasses.dataclass
class _S3UploadPart:
    data: bytearray
    meta: Mapping[str, str]


@dataclasses.dataclass
class _S3Upload:
    parts: MutableMapping[int, _S3UploadPart]
    meta: Mapping[str, str]


@dataclasses.dataclass
class _S3BucketUploadStorage:
    def __init__(self) -> None:
        self._storage: dict[str, _S3Upload] = {}

    @staticmethod
    def _generate_upload_id():
        return os.urandom(15).hex()

    @staticmethod
    def _generate_etag(data):
        return hashlib.md5(data).hexdigest()

    def create_multipart_upload(self, key: str, user_defined_meta: Mapping[str, str] | None = None):
        key_path = pathlib.Path(key)
        upload_id = _S3BucketUploadStorage._generate_upload_id()

        upload_meta = {
            'Key': str(key_path),
            'UploadId': upload_id,
        }

        if user_defined_meta:
            upload_meta.update(user_defined_meta)

        self._storage[upload_id] = _S3Upload(parts={}, meta=upload_meta)
        return upload_meta

    def abort_multipart_uplod(self, key: str, upload_id: str):
        key_path = pathlib.Path(key)
        upload = self._storage.get(upload_id)
        if not upload or upload.meta['Key'] != str(key_path):
            raise _S3NoSuchUploadError()
        return self._storage.pop(upload_id)

    def upload_part(
        self,
        key: str,
        upload_id: str,
        part_number: int,
        data: bytearray,
        last_modified: dt.datetime | str | None = None,
    ):
        if part_number < 1 or part_number > 10000:
            raise _S3ClientError('partNumber value is expected to be between 1 and 10000')

        key_path = pathlib.Path(key)
        upload = self._storage.get(upload_id)
        if not upload or upload.meta['Key'] != str(key_path):
            raise _S3NoSuchUploadError()

        if last_modified is None:
            # Timezone is needed for RFC 3339 timeformat used by S3
            last_modified = dt.datetime.now().replace(tzinfo=tz.tzlocal()).isoformat()
        elif isinstance(last_modified, dt.datetime):
            last_modified = last_modified.isoformat()

        meta = {
            'ETag': self._generate_etag(data),
            'Last-Modified': last_modified,
            'Size': str(sys.getsizeof(data)),
        }

        new_part = _S3UploadPart(data, meta)
        upload.parts[part_number] = new_part
        return new_part

    def complete_multipart_upload(self, key: str, upload_id: str, parts_to_complete: list):
        key_path = pathlib.Path(key)
        upload = self._storage.get(upload_id)

        if not upload or upload.meta['Key'] != str(key_path):
            raise _S3NoSuchUploadError()

        uploaded_parts = sorted(
            ({'PartNumber': part_number, 'ETag': info.meta['ETag']} for part_number, info in upload.parts.items()),
            key=lambda item: item['PartNumber'],
        )
        if uploaded_parts != parts_to_complete:
            raise _S3InvalidPartOrderError()

        merged_data = bytearray()
        for part in parts_to_complete:
            part_number = part['PartNumber']
            uploded_part = upload.parts[part_number]
            merged_data += uploded_part.data

        if not merged_data:
            raise _S3EntityTooSmallError()

        self._storage.pop(upload_id)
        return {'Data': merged_data, 'Upload': upload}


@dataclasses.dataclass
class S3Object:
    data: bytearray
    meta: Mapping[str, str]


class S3MockBucketStorage:
    def __init__(self) -> None:
        # use Path to normalize keys (e.g. /a//file.json)
        self._storage: dict[pathlib.Path, S3Object] = {}

    @staticmethod
    def _generate_etag(data):
        return hashlib.md5(data).hexdigest()

    def put_object(
        self,
        key: str,
        data: bytearray,
        user_defined_meta: Mapping[str, str] | None = None,
        last_modified: dt.datetime | str | None = None,
    ):
        key_path = pathlib.Path(key)
        if last_modified is None:
            # Timezone is needed for RFC 3339 timeformat used by S3
            last_modified = dt.datetime.now().replace(tzinfo=tz.tzlocal()).isoformat()
        elif isinstance(last_modified, dt.datetime):
            last_modified = last_modified.isoformat()

        meta = {
            'Key': str(key_path),
            'ETag': self._generate_etag(data),
            'Last-Modified': last_modified,
            'Size': str(sys.getsizeof(data)),
        }

        if user_defined_meta:
            meta.update(user_defined_meta)

        self._storage[key_path] = S3Object(data, meta)
        return meta

    def get_object(self, key: str) -> S3Object | None:
        key_path = pathlib.Path(key)
        return self._storage.get(key_path)

    def get_objects(self, parent_dir='') -> dict[str, S3Object]:
        all_objects = {str(key_path): value for key_path, value in self._storage.items()}

        if not parent_dir:
            return all_objects

        return {key: value for key, value in all_objects.items() if key.startswith(str(pathlib.Path(parent_dir)))}

    def delete_object(self, key) -> S3Object | None:
        key = pathlib.Path(key)
        if key not in self._storage:
            return None
        return self._storage.pop(key)


class S3HandleMock:
    _s3_xml_nss = {'s3': 'http://s3.amazonaws.com/doc/2006-03-01/'}

    def __init__(self, mockserver, s3_mock_storage, mock_base_url):
        self._mockserver = mockserver
        self._base_url = mock_base_url
        self._storage = s3_mock_storage
        self._uploads = collections.defaultdict(_S3BucketUploadStorage)

    def _get_bucket_name(self, request):
        return request.headers['Host'].split('.')[0]

    def _extract_key(self, request):
        return request.path[len(self._base_url) + 1 :]

    def _generate_get_objects_result(
        self,
        s3_objects_dict: dict[str, S3Object],
        max_keys: int,
        marker: str | None,
    ):
        empty_result = {'result_objects': [], 'is_truncated': False}
        keys = list(s3_objects_dict.keys())
        if not keys:
            return empty_result

        from_index = 0
        if marker:
            if marker > keys[-1]:
                return empty_result
            for i, key in enumerate(keys):
                if key > marker:
                    from_index = i
                    break

        result_objects = [s3_objects_dict[key] for key in keys[from_index : from_index + max_keys]]
        is_truncated = from_index + max_keys >= len(keys)
        return {'result_objects': result_objects, 'is_truncated': is_truncated}

    def _generate_get_objects_xml(
        self,
        s3_objects: list[S3Object],
        bucket_name: str,
        prefix: str,
        max_keys: int | None,
        marker: str | None,
        is_truncated: bool,
    ):
        contents = ''
        for s3_object in s3_objects:
            contents += f"""
                <Contents>
                    <ETag>{s3_object.meta['ETag']}</ETag>
                    <Key>{s3_object.meta['Key']}</Key>
                    <LastModified>{s3_object.meta['Last-Modified']}</LastModified>
                    <Size>{s3_object.meta['Size']}</Size>
                    <StorageClass>STANDARD</StorageClass>
                </Contents>
                """
        return f"""
            <?xml version="1.0" encoding="UTF-8"?>
                <ListBucketResult>
                <Name>{bucket_name}</Name>
                <Prefix>{prefix}</Prefix>
                <Marker>{marker or ''}</Marker>
                <MaxKeys>{max_keys or ''}</MaxKeys>
                <IsTruncated>{is_truncated}</IsTruncated>
                {contents}
            </ListBucketResult>
            """

    @staticmethod
    def _generate_error_response_xml(code: str, message: str, resource: str):
        return (
            '<?xml version="1.0" encoding="UTF-8"?>'
            '<Error>'
            f'<Code>{code}</Code>'
            f'<Message>{message}</Message>'
            f'<Resource>{resource}</Resource>'
            f'<RequestId>{os.urandom(15).hex()}</RequestId>'
            '</Error>'
        )

    @staticmethod
    def _parse_complete_multipart_xml_body(request_body: str):
        xml_root_node = xml.etree.ElementTree.fromstring(request_body)
        if xml_root_node is None or xml_root_node.tag != f'{{{S3HandleMock._s3_xml_nss["s3"]}}}CompleteMultipartUpload':
            raise _S3ClientError('missing CompleteMultipartUpload in request body')

        parts_to_complete = []
        for xml_part in xml_root_node.findall('s3:Part', S3HandleMock._s3_xml_nss):
            xml_part_number = xml_part.find('s3:PartNumber', S3HandleMock._s3_xml_nss)
            if xml_part_number is None or not xml_part_number.text:
                raise _S3ClientError('missing CompleteMultipartUpload.Part.PartNumber')
            part_number_value = int(xml_part_number.text)

            xml_etag = xml_part.find('s3:ETag', S3HandleMock._s3_xml_nss)
            if xml_etag is None or not xml_etag.text:
                raise _S3ClientError('missing CompleteMultipartUpload.Part.ETag')

            parts_to_complete.append({'ETag': xml_etag.text, 'PartNumber': part_number_value})

        return parts_to_complete

    def get_object(self, request):
        key = self._extract_key(request)

        bucket_storage = self._storage[self._get_bucket_name(request)]

        s3_object = bucket_storage.get_object(key)
        if not s3_object:
            return self._mockserver.make_response('Object not found', 404)
        return self._mockserver.make_response(
            s3_object.data,
            200,
            headers=s3_object.meta,
        )

    def put_object(self, request):
        key = self._extract_key(request)

        bucket_storage = self._storage[self._get_bucket_name(request)]

        data = request.get_data()

        user_defined_meta = {}
        for meta_key, meta_value in request.headers.items():
            # https://docs.amazonaws.cn/en_us/AmazonS3/latest/userguide/UsingMetadata.html
            if meta_key.startswith('x-amz-meta-') or meta_key in ['Content-Type', 'Content-Disposition']:
                user_defined_meta[meta_key] = meta_value

        meta = bucket_storage.put_object(key, data, user_defined_meta)
        # Some clients like AWS SDK for C++ parse not empty body as XML
        return self._mockserver.make_response('', 200, headers=meta)

    def copy_object(self, request):
        key = self._extract_key(request)
        dest_bucket_name = self._get_bucket_name(request)
        source_bucket_name, source_key = request.headers.get(
            'x-amz-copy-source',
        ).split('/', 2)[1:3]

        src_bucket_storage = self._storage[source_bucket_name]
        dst_bucket_storage = self._storage[dest_bucket_name]

        src_obj = src_bucket_storage.get_object(source_key)
        src_data = src_obj.data
        src_meta = src_obj.meta
        meta = dst_bucket_storage.put_object(key, src_data, src_meta)
        # Some clients like AWS SDK for C++ parse not empty body as XML
        return self._mockserver.make_response('', 200, headers=meta)

    def get_objects(self, request):
        prefix = request.query['prefix']
        # 1000 is the default value specified by aws spec
        max_keys = int(request.query.get('max-keys', 1000))
        marker = request.query.get('marker')

        bucket_name = self._get_bucket_name(request)
        bucket_storage = self._storage[bucket_name]

        s3_objects_dict = bucket_storage.get_objects(parent_dir=prefix)
        result = self._generate_get_objects_result(
            s3_objects_dict=s3_objects_dict,
            max_keys=max_keys,
            marker=marker,
        )
        result_xml = self._generate_get_objects_xml(
            s3_objects=result['result_objects'],
            bucket_name=bucket_name,
            prefix=prefix,
            max_keys=max_keys,
            marker=marker,
            is_truncated=result['is_truncated'],
        )
        return self._mockserver.make_response(result_xml, 200)

    def delete_object(self, request):
        key = self._extract_key(request)

        bucket_storage = self._storage[self._get_bucket_name(request)]

        bucket_storage.delete_object(key)
        # S3 always return 204, even if file doesn't exist
        # Some clients like AWS SDK for C++ parse not empty body as XML
        return self._mockserver.make_response('', 204)

    def get_object_head(self, request):
        key = self._extract_key(request)

        bucket_storage = self._storage[self._get_bucket_name(request)]

        s3_object = bucket_storage.get_object(key)
        if not s3_object:
            return self._mockserver.make_response('Object not found', 404)
        # Some clients like AWS SDK for C++ parse not empty body as XML
        return self._mockserver.make_response(
            '',
            200,
            headers=s3_object.meta,
        )

    def create_multipart_upload(self, request):
        key = self._extract_key(request)
        bucket_name = self._get_bucket_name(request)
        bucket_uploads = self._uploads[bucket_name]

        user_defined_meta = {}
        for meta_key, meta_value in request.headers.items():
            # https://docs.amazonaws.cn/en_us/AmazonS3/latest/userguide/UsingMetadata.html
            if meta_key.startswith('x-amz-meta-') or meta_key in ['Content-Type', 'Content-Disposition']:
                user_defined_meta[meta_key] = meta_value

        meta = bucket_uploads.create_multipart_upload(key, user_defined_meta)
        response_body = (
            '<?xml version="1.0" encoding="UTF-8"?>'
            '<InitiateMultipartUploadResult>'
            f'<Bucket>{bucket_name}</Bucket>'
            f'<Key>{key}</Key>'
            f'<UploadId>{meta["UploadId"]}</UploadId>'
            '</InitiateMultipartUploadResult>'
        )
        # Some clients like AWS SDK for C++ parse not empty body as XML
        return self._mockserver.make_response(response_body, 200)

    def abort_multipart_upload(self, request):
        key = self._extract_key(request)
        upload_id = request.query['uploadId']
        bucket_uploads = self._uploads[self._get_bucket_name(request)]
        try:
            bucket_uploads.abort_multipart_uplod(key, upload_id)
        except _S3NoSuchUploadError as exc:
            # https://docs.aws.amazon.com/AmazonS3/latest/API/API_AbortMultipartUpload.html
            # #API_AbortMultipartUpload_Errors
            response_body = S3HandleMock._generate_error_response_xml(
                exc.code, exc.message, f'{request.path}?uploadId={upload_id}'
            )
            return self._mockserver.make_response(response_body, 404)

        # Some clients like AWS SDK for C++ parse not empty body as XML
        return self._mockserver.make_response('', 204)

    def upload_part(self, request):
        key = self._extract_key(request)
        bucket_name = self._get_bucket_name(request)
        upload_id = request.query['uploadId']
        part_number = int(request.query['partNumber'])
        bucket_uploads = self._uploads[bucket_name]
        data = request.get_data()
        try:
            upload_part = bucket_uploads.upload_part(key, upload_id, part_number, data)
        except _S3ClientError as exc:
            return self._mockserver.make_response(str(exc), 400)
        except _S3NoSuchUploadError as exc:
            # https://docs.aws.amazon.com/AmazonS3/latest/API/API_UploadPart.html
            response_body = S3HandleMock._generate_error_response_xml(
                exc.code,
                exc.message,
                f'{request.path}?uploadId={upload_id}',
            )
            return self._mockserver.make_response(response_body, 404)

        return self._mockserver.make_response(status=200, headers={'ETag': upload_part.meta['ETag']})

    def complete_multipart_upload(self, request):
        key = self._extract_key(request)
        bucket_name = self._get_bucket_name(request)
        bucket_uploads = self._uploads[bucket_name]
        bucket_storage = self._storage[bucket_name]
        upload_id = request.query['uploadId']
        try:
            parts_to_complete = S3HandleMock._parse_complete_multipart_xml_body(request.get_data().decode())
            completed_upload = bucket_uploads.complete_multipart_upload(key, upload_id, parts_to_complete)
        except _S3NoSuchUploadError as exc:
            # https://docs.aws.amazon.com/AmazonS3/latest/API/API_CompleteMultipartUpload.html
            response_body = S3HandleMock._generate_error_response_xml(
                exc.code,
                exc.message,
                f'{request.path}?uploadId={upload_id}',
            )
            return self._mockserver.make_response(response_body, 404)
        except (_S3InvalidPartError, _S3InvalidPartOrderError, _S3EntityTooSmallError) as exc:
            # https://docs.aws.amazon.com/AmazonS3/latest/API/API_CompleteMultipartUpload.html
            response_body = S3HandleMock._generate_error_response_xml(
                exc.code,
                exc.message,
                f'{request.path}?uploadId={upload_id}',
            )
            return self._mockserver.make_response(response_body, 400)
        except _S3ClientError as exc:
            return self._mockserver.make_response(str(exc), 400)

        meta = bucket_storage.put_object(key, completed_upload['Data'], completed_upload['Upload'].meta)
        response_body = (
            '<?xml version="1.0" encoding="UTF-8"?>'
            '<CompleteMultipartUploadResult>'
            f'<Location>{request.path}</Location>'
            f'<Bucket>{bucket_name}</Bucket>'
            f'<Key>{key}</Key>'
            f'<ETag>{meta["ETag"]}</ETag>'
            '</CompleteMultipartUploadResult>'
        )
        return self._mockserver.make_response(
            response_body,
            status=200,
            headers=meta,
        )

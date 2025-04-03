import dataclasses
import datetime as dt
import hashlib
import pathlib
import sys
from typing import Dict
from typing import List
from typing import Mapping
from typing import Optional
from typing import Union

import dateutil.tz as tz


@dataclasses.dataclass
class S3Object:
    data: bytearray
    meta: Mapping[str, str]


class S3MockBucketStorage:
    def __init__(self):
        # use Path to normalize keys (e.g. /a//file.json)
        self._storage: Dict[pathlib.Path, S3Object] = {}

    @staticmethod
    def _generate_etag(data):
        return hashlib.md5(data).hexdigest()

    def put_object(
        self,
        key: str,
        data: bytearray,
        user_defined_meta: Mapping[str, str] = {},
        last_modified: Optional[Union[dt.datetime, str]] = None,
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

        meta.update(user_defined_meta)

        self._storage[key_path] = S3Object(data, meta)
        return meta

    def get_object(self, key: str) -> Optional[S3Object]:
        key_path = pathlib.Path(key)
        return self._storage.get(key_path)

    def get_objects(self, parent_dir='') -> Dict[str, S3Object]:
        all_objects = {str(key_path): value for key_path, value in self._storage.items()}

        if not parent_dir:
            return all_objects

        return {key: value for key, value in all_objects.items() if key.startswith(str(pathlib.Path(parent_dir)))}

    def delete_object(self, key) -> Optional[S3Object]:
        key = pathlib.Path(key)
        if key not in self._storage:
            return None
        return self._storage.pop(key)


class S3HandleMock:
    def __init__(self, mockserver, s3_mock_storage, mock_base_url):
        self._mockserver = mockserver
        self._base_url = mock_base_url
        self._storage = s3_mock_storage

    def _get_bucket_name(self, request):
        return request.headers['Host'].split('.')[0]

    def _extract_key(self, request):
        return request.path[len(self._base_url) + 1 :]

    def _generate_get_objects_result(
        self,
        s3_objects_dict: Dict[str, S3Object],
        max_keys: int,
        marker: Optional[str],
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
        s3_objects: List[S3Object],
        bucket_name: str,
        prefix: str,
        max_keys: Optional[int],
        marker: Optional[str],
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
            if meta_key.startswith('x-amz-meta-'):
                user_defined_meta[meta_key] = meta_value

        meta = bucket_storage.put_object(key, data, user_defined_meta)
        return self._mockserver.make_response('OK', 200, headers=meta)

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
        return self._mockserver.make_response('OK', 200, headers=meta)

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
        return self._mockserver.make_response('OK', 204)

    def get_object_head(self, request):
        key = self._extract_key(request)

        bucket_storage = self._storage[self._get_bucket_name(request)]

        s3_object = bucket_storage.get_object(key)
        if not s3_object:
            return self._mockserver.make_response('Object not found', 404)
        return self._mockserver.make_response(
            'OK',
            200,
            headers=s3_object.meta,
        )

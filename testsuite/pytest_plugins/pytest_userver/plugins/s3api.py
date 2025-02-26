import collections

import pytest

from pytest_userver import s3api

pytest_plugins = ['pytest_userver.plugins.core']


def pytest_configure(config):
    config.addinivalue_line('markers', 's3: store s3 files in mock')


@pytest.fixture(name='s3_mock_storage')
def _s3_mock_storage():
    buckets = collections.defaultdict(s3api.S3MockBucketStorage)
    return buckets


@pytest.fixture(autouse=True)
def s3_mock(mockserver, s3_mock_storage):
    mock_base_url = '/mds-s3'
    mock_impl = s3api.S3HandleMock(
        mockserver=mockserver,
        s3_mock_storage=s3_mock_storage,
        mock_base_url=mock_base_url,
    )

    @mockserver.handler(mock_base_url, prefix=True)
    def _mock_all(request):
        if request.method == 'GET':
            if 'prefix' in request.query:
                return mock_impl.get_objects(request)
            return mock_impl.get_object(request)

        if request.method == 'PUT':
            if request.headers.get('x-amz-copy-source', None):
                return mock_impl.copy_object(request)
            return mock_impl.put_object(request)

        if request.method == 'DELETE':
            return mock_impl.delete_object(request)

        if request.method == 'HEAD':
            return mock_impl.get_object_head(request)

        return mockserver.make_response('Unknown or unsupported method', 404)


@pytest.fixture(autouse=True)
def s3_apply(request, s3_mock_storage, load):
    def _put_files(bucket, files):
        bucket_storage = s3_mock_storage[bucket]
        for s3_path, file_path in files.items():
            bucket_storage.put_object(
                key=s3_path,
                data=load(file_path).encode('utf-8'),
            )

    for mark in request.node.iter_markers('s3'):
        _put_files(*mark.args, **mark.kwargs)

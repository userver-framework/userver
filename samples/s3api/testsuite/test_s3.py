import pytest


# /// [s3 mark]
@pytest.mark.s3('mybucket0', {'some_path/to/an/object': 'preset.txt'})
async def test_s3_marker_preloads(s3_mock_storage, load):
    obj = s3_mock_storage['mybucket0'].get_object('some_path/to/an/object')
    assert obj is not None
    assert obj.data == load('preset.txt').encode()
    # /// [s3 mark]


# /// [s3 mock]
async def test_sample_component_data_write(s3_mock_storage, service_client):
    bucket = 'mybucket'
    object_path = 'path/to/object'
    data = b'some string data from S3ApiSampleComponent start'

    assert s3_mock_storage[bucket].get_object(object_path) is None

    await service_client.run_task('some-s3-fill-task')
    stored = s3_mock_storage[bucket].get_object(object_path)
    assert stored.data == data
    # /// [s3 mock]

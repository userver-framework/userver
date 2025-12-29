import xml.etree.ElementTree

import aiohttp


def _create_s3_url(mockserver, key):
    return mockserver.url(f'/mds-s3/{key}')


def _create_s3_basic_heades(mockserver, bucket_name):
    return {'Host': f'{bucket_name}.{mockserver.host}:{mockserver.port}'}


class _CustomClientException(Exception):
    def __init__(self, status, body):
        self.status = status
        self.body = body


def _try_get_error_code(body_str):
    xml_root_node = xml.etree.ElementTree.fromstring(body_str)
    assert xml_root_node is not None and xml_root_node.tag == 'Error'
    xml_err_code_node = xml_root_node.find('Code')
    if xml_err_code_node is not None:
        return xml_err_code_node.text
    return None


async def create_multipart_upload(mockserver, bucket_name, key, content_type=None, content_disposition=None):
    url = _create_s3_url(mockserver, key)
    headers = _create_s3_basic_heades(mockserver, bucket_name)
    if content_type:
        headers['Content-Type'] = content_type
    if content_disposition:
        headers['Content-Disposition'] = content_disposition
    async with aiohttp.ClientSession() as session:
        async with session.post(url, headers=headers, params='uploads') as response:
            response.raise_for_status()
            body = await response.text()
    assert body is not None
    xml_root_node = xml.etree.ElementTree.fromstring(body)
    assert xml_root_node.tag == 'InitiateMultipartUploadResult'
    xml_bucket_node = xml_root_node.find('Bucket')
    xml_key_node = xml_root_node.find('Key')
    xml_upload_id = xml_root_node.find('UploadId')
    assert xml_bucket_node is not None
    assert xml_key_node is not None
    assert xml_upload_id is not None
    assert xml_bucket_node.text == bucket_name
    assert xml_key_node.text == key
    assert xml_upload_id.text is not None
    return xml_upload_id.text


async def upload_part(mockserver, bucket_name, key, upload_id, part_number, data):
    url = _create_s3_url(mockserver, key)
    headers = _create_s3_basic_heades(mockserver, bucket_name)
    if isinstance(data, str):
        data_bytes = data.encode()
    elif isinstance(data, bytearray):
        data_bytes = data
    else:
        raise RuntimeError('`data` is expected to be a text or bytearray')
    response_body = None
    async with aiohttp.ClientSession() as session:
        async with session.put(
            url, headers=headers, params={'uploadId': upload_id, 'partNumber': part_number}, data=data_bytes
        ) as response:
            if response.status in {200, 400, 404}:
                response_body = await response.text()
                if response.status != 200 and response_body is not None:
                    raise _CustomClientException(response.status, response_body)
            else:
                response.raise_for_status()
    assert response_body is not None
    etag = response.headers.get('ETag')
    assert etag is not None
    return etag


async def abort_multipart_upload(mockserver, bucket_name, key, upload_id):
    url = _create_s3_url(mockserver, key)
    headers = _create_s3_basic_heades(mockserver, bucket_name)
    async with aiohttp.ClientSession() as session:
        async with session.delete(url, headers=headers, params={'uploadId': upload_id}) as response:
            if response.status in {400, 404}:
                response_body = await response.text()
                if response_body is not None:
                    raise _CustomClientException(response.status, response_body)
            else:
                response.raise_for_status()


async def complete_multipart_upload(mockserver, bucket_name, key, upload_id, parts):
    url = _create_s3_url(mockserver, key)
    headers = _create_s3_basic_heades(mockserver, bucket_name)
    body_str = str('<?xml version="1.0"?><CompleteMultipartUpload xmlns="http://s3.amazonaws.com/doc/2006-03-01/">')
    for part in parts:
        body_str += f'<Part><PartNumber>{part[0]}</PartNumber><ETag>{part[1]}</ETag></Part>'
    body_str += '</CompleteMultipartUpload>'
    response_body = None
    async with aiohttp.ClientSession() as session:
        async with session.post(
            url, headers=headers, params={'uploadId': upload_id}, data=body_str.encode()
        ) as response:
            if response.status in {200, 400, 404}:
                response_body = await response.text()
                if response.status != 200 and response_body is not None:
                    raise _CustomClientException(response.status, response_body)
            else:
                response.raise_for_status()
    assert response_body is not None
    xml_root_node = xml.etree.ElementTree.fromstring(response_body)
    assert xml_root_node is not None and xml_root_node.tag == 'CompleteMultipartUploadResult'
    xml_bucket_node = xml_root_node.find('Bucket')
    xml_key_node = xml_root_node.find('Key')
    xml_location_node = xml_root_node.find('Location')
    xml_etag_node = xml_root_node.find('ETag')
    assert xml_bucket_node is not None
    assert xml_key_node is not None
    assert xml_location_node is not None
    assert xml_etag_node is not None
    assert xml_bucket_node.text == bucket_name
    assert xml_key_node.text == key
    return {'ETag': xml_bucket_node.text, 'Bucket': xml_bucket_node.text, 'Key': xml_key_node.text}


async def test_successfull_upload(s3_mock, s3_mock_storage, mockserver):
    data = 'the first data part; the second data part; the third data part'
    bucket_name = 'some-bucket'
    key = 'some/key'
    upload_id = await create_multipart_upload(mockserver, bucket_name, key)
    etag1 = await upload_part(mockserver, bucket_name, key, upload_id, 1, data[0:19])
    etag2 = await upload_part(mockserver, bucket_name, key, upload_id, 2, data[19:41])
    etag3 = await upload_part(mockserver, bucket_name, key, upload_id, 3, data[41:])
    await complete_multipart_upload(mockserver, bucket_name, key, upload_id, [(1, etag1), (2, etag2), (3, etag3)])
    stored_data = s3_mock_storage[bucket_name].get_object(key).data
    assert stored_data == data.encode()


async def test_invalid_completion(s3_mock, mockserver):
    data = 'the first data part; the second data part; the third data part'
    bucket_name = 'some-bucket'
    key = 'some/key'
    upload_id = await create_multipart_upload(mockserver, bucket_name, key)
    etag1 = await upload_part(mockserver, bucket_name, key, upload_id, 1, data[0:19])
    etag2 = await upload_part(mockserver, bucket_name, key, upload_id, 2, data[19:41])
    etag3 = await upload_part(mockserver, bucket_name, key, upload_id, 3, data[41:])
    try:
        await complete_multipart_upload(mockserver, bucket_name, key, upload_id, [(3, etag1), (2, etag2), (1, etag3)])
    except _CustomClientException as e:
        assert e.status == 400
        assert _try_get_error_code(e.body) == 'InvalidPartOrder'
    else:
        assert False, 'the last request is expected to raise an exception'

    try:
        await complete_multipart_upload(mockserver, bucket_name, key, upload_id, [])
    except _CustomClientException as e:
        assert e.status == 400
        assert _try_get_error_code(e.body) == 'InvalidPartOrder'
    else:
        assert False, 'the last request is expected to raise an exception'

    try:
        await complete_multipart_upload(mockserver, bucket_name, key, upload_id, [])
    except _CustomClientException as e:
        assert e.status == 400
        assert _try_get_error_code(e.body) == 'InvalidPartOrder'
    else:
        assert False, 'the last request is expected to raise an exception'

    try:
        await complete_multipart_upload(
            mockserver, bucket_name, key, 'unknown_upload', [(1, etag1), (2, etag2), (3, etag3)]
        )
    except _CustomClientException as e:
        assert e.status == 404
        assert _try_get_error_code(e.body) == 'NoSuchUpload'
    else:
        assert False, 'the last request is expected to raise an exception'


async def test_empty_object(s3_mock, mockserver):
    bucket_name = 'some-bucket'
    key = 'some/key1'
    upload_id = await create_multipart_upload(mockserver, bucket_name, key)
    etag1 = await upload_part(mockserver, bucket_name, key, upload_id, 1, '')
    try:
        await complete_multipart_upload(mockserver, bucket_name, key, upload_id, [(1, etag1)])
    except _CustomClientException as e:
        assert e.status == 400
        assert _try_get_error_code(e.body) == 'EntityTooSmall'
    else:
        assert False, 'the last request is expected to raise an exception'

    key = 'some/key2'
    upload_id = await create_multipart_upload(mockserver, bucket_name, key)
    try:
        await complete_multipart_upload(mockserver, bucket_name, key, upload_id, [])
    except _CustomClientException as e:
        assert e.status == 400
        assert _try_get_error_code(e.body) == 'EntityTooSmall'
    else:
        assert False, 'the last request is expected to raise an exception'


async def test_failed_upload_part(s3_mock, mockserver):
    data = 'the first data part; the second data part; the third data part'
    bucket_name = 'some-bucket'
    key = 'some/key'
    await create_multipart_upload(mockserver, bucket_name, key)
    try:
        await upload_part(mockserver, bucket_name, key, 'unknown_upload', 1, data[0:19])
    except _CustomClientException as e:
        assert e.status == 404
        assert _try_get_error_code(e.body) == 'NoSuchUpload'
    else:
        assert False, 'the last request is expected to raise an exception'


async def test_abort_upload(s3_mock, s3_mock_storage, mockserver):
    data = 'the first data part; the second data part; the third data part'
    bucket_name = 'some-bucket'
    key = 'some/key'
    upload_id = await create_multipart_upload(mockserver, bucket_name, key)
    etag1 = await upload_part(mockserver, bucket_name, key, upload_id, 1, data[0:19])
    etag2 = await upload_part(mockserver, bucket_name, key, upload_id, 2, data[19:41])
    etag3 = await upload_part(mockserver, bucket_name, key, upload_id, 3, data[41:])
    await abort_multipart_upload(mockserver, bucket_name, key, upload_id)
    try:
        await complete_multipart_upload(mockserver, bucket_name, key, upload_id, [(1, etag1), (2, etag2), (3, etag3)])
    except _CustomClientException as e:
        assert e.status == 404
        assert _try_get_error_code(e.body) == 'NoSuchUpload'
    else:
        assert False, f"upload '{upload_id}' had been aborted before"

    assert s3_mock_storage[bucket_name].get_object(key) is None

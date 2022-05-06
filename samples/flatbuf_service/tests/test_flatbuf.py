# /// [Functional test]
async def test_flatbuf(service_client):
    body = bytearray.fromhex(
        '100000000c00180000000800100004000c000000140000001400000000000000'
        '16000000000000000a00000048656c6c6f20776f72640000',
    )
    response = await service_client.post('/fbs', data=body)
    assert response.status == 200
    # /// [Functional test]

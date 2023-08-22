import pytest

# /// [Functional test]

async def test_bitstring_simple(service_client):
    response = await service_client.post('/v1/bitstring?id=1&value=11110010&db_type=bit8&cpp_type=bitset8')
    assert response.status == 200
    assert response.text == "11110010"

async def post_bitstring(service_client, id, value, cpp_type, db_type):
    return await service_client.post(
        '/v1/bitstring',
        params={
            'id': id,
            'value': value,
            'cpp_type': cpp_type,
            'db_type': db_type,
        },
    )

async def post_bitstring_test(service_client, id, value, cpp_type, db_type, code, result_len):
    response = await post_bitstring(service_client, id, value, cpp_type, db_type)
    assert response.status == code
    assert code != 200 or response.text == value.ljust(result_len, "0")

@pytest.mark.parametrize('id, value, cpp_type, db_type, code, result_len', [
    ('1', '01001111', 'bitset8', 'varbit', 200, 8),
    ('2', '01001111', 'bitset32', 'varbit', 200, 32),
    ('3', '01001111', 'bitset128', 'varbit', 200, 128),
    ('4', '01001111', 'bitset8', 'bit8', 200, 8),
    ('5', '01001111', 'bitset8', 'bit32', 500, 8),
    ('6', '01001111', 'bitset32', 'bit32', 200, 32),
    ('7', '01001111', 'bitset8', 'bit128', 500, 8),
    ('8', '01001111', 'bitset32', 'bit128', 500, 32),
    ('9', '01001111', 'bitset128', 'bit128', 200, 128)
])
async def test_post_bitset(service_client, id, value, cpp_type, db_type, code, result_len):
    await post_bitstring_test(service_client, id, value, cpp_type, db_type, code, result_len)

@pytest.mark.parametrize('id, value, cpp_type, db_type, code, result_len', [
    ('1', '01001111', 'uint8', 'varbit', 200, 8),
    ('2', '01001111', 'uint32', 'varbit', 200, 32),
    ('4', '01001111', 'uint8', 'bit8', 200, 8),
    ('5', '01001111', 'uint8', 'bit32', 500, 8),
    ('6', '01001111', 'uint32', 'bit32', 200, 32),
])
async def test_post_uint(service_client, id, value, cpp_type, db_type, code, result_len):
    await post_bitstring_test(service_client, id, value, cpp_type, db_type, code, result_len)

@pytest.mark.parametrize('id, value, cpp_type, db_type, code, result_len', [
    ('1', '01001111', 'flags8', 'varbit', 200, 32), # attention!
    ('2', '01001111', 'flags8', 'bit8', 500, 32),   # attention!
    ('3', '01001111', 'flags8', 'bit32', 200, 32),
    ('4', '01001111', 'flags8', 'bit128', 500, 32),
])
async def test_post_flags(service_client, id, value, cpp_type, db_type, code, result_len):
    await post_bitstring_test(service_client, id, value, cpp_type, db_type, code, result_len)

async def get_bitstring(service_client, id, cpp_type, db_type):
    return await service_client.get(
        '/v1/bitstring',
        params={
            'id': id,
            'cpp_type': cpp_type,
            'db_type': db_type,
        },
    )

async def get_bitstring_test(service_client, id, value, db_type, code, cpp_type, result_len):
    response = await get_bitstring(service_client, id, cpp_type, db_type)
    assert response.status == code
    assert code != 200 or response.text == value.ljust(result_len, "0")

@pytest.mark.pgsql('db_1', files=['initial_data.sql'])
@pytest.mark.parametrize('id, value, db_type, cpp_type, code, result_len', [
    ('1', '01001111', 'varbit', 'bitset8', 200, 8),
    ('1', '01001111', 'varbit', 'bitset32', 200, 32),
    ('2', '01001111', 'varbit', 'bitset8', 500, 8),
    ('1', '01001111', 'bit8', 'bitset8', 200, 8),
    ('1', '01001111', 'bit8', 'bitset32', 200, 32),
    ('1', '01001111', 'bit32', 'bitset32', 200, 32),
    ('1', '01001111', 'bit32', 'bitset8', 500, 8),
])
async def test_get_bitset(service_client, id, value, db_type, code, cpp_type, result_len):
    await get_bitstring_test(service_client, id, value, db_type, code, cpp_type, result_len)

@pytest.mark.pgsql('db_1', files=['initial_data.sql'])
@pytest.mark.parametrize('id, value, db_type, cpp_type, code, result_len', [
    ('1', '01001111', 'varbit', 'uint8', 200, 8),
    ('1', '01001111', 'varbit', 'uint32', 200, 32),
    ('2', '01001111', 'varbit', 'uint32', 200, 32),
    ('2', '01001111', 'varbit', 'uint8', 500, 8),
    ('1', '01001111', 'bit8', 'uint8', 200, 8),
    ('1', '01001111', 'bit8', 'uint32', 200, 32),
    ('1', '01001111', 'bit32', 'uint32', 200, 32),
    ('1', '01001111', 'bit32', 'uint8', 500, 8),
])
async def test_get_uint(service_client, id, value, db_type, code, cpp_type, result_len):
    await get_bitstring_test(service_client, id, value, db_type, code, cpp_type, result_len)

@pytest.mark.pgsql('db_1', files=['initial_data.sql'])
@pytest.mark.parametrize('id, value, db_type, cpp_type, code, result_len', [
    ('2', '01001111', 'varbit', 'flags8', 200, 32),
    ('1', '01001111', 'varbit', 'flags8', 200, 32),
    ('3', '01001111', 'varbit', 'flags8', 500, 32),
    ('1', '01001111', 'bit32', 'flags8', 200, 32),
    ('1', '01001111', 'bit8', 'flags8', 200, 32),
    ('1', '01001111', 'bit128', 'flags8', 500, 32),
])
async def test_get_flags(service_client, id, value, db_type, code, cpp_type, result_len):
    await get_bitstring_test(service_client, id, value, db_type, code, cpp_type, result_len)

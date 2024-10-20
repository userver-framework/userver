import pytest

@pytest.mark.parametrize(
    'order, expected',
    (
        pytest.param(
            'asc',
            'one3',
            id='asc',
        ),
        pytest.param(
            'desc',
            'one1',
            id='desc',
        ),
    )
)
async def test_cache_order_by(service_client, pgsql, order, expected):
    cursor = pgsql['key_value'].cursor()
    cursor.execute('''
    INSERT INTO key_value_table (
        key, 
        value,
        updated
    ) VALUES (
        'one',
        'one1',
        '2024-01-01'
    ), (
        'one',
        'one2',
        '2024-01-02'
    ), (
        'one',
        'one3',
        '2024-01-03'
    ), (
        'two',
        'two1',
        '2024-01-04'
    )
    ''')
    response = await service_client.get('/cache/order-by', params={
        'key': 'one',
        'order': order,
    })
    assert response.status == 200
    assert response.json() == {
        "result": expected,
    }

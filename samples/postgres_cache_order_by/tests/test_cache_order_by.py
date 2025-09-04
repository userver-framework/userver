import pytest


@pytest.mark.pgsql('key_value', files=['test_data.sql'])
@pytest.mark.parametrize(
    'order, expected',
    (
        pytest.param('last', 'one3', id='last'),
        pytest.param('first', 'one1', id='first'),
    ),
)
async def test_cache_order_by(service_client, order, expected):
    response = await service_client.get(
        '/cache/order-by',
        params={
            'key': 'one',
            'order': order,
        },
    )
    assert response.status == 200
    assert response.json() == {'result': expected}

import dateutil
import pytest


# /// [mocked_time]
@pytest.mark.now('2019-12-31T11:22:33Z')
async def test_now(service_client, mocked_time):
    response = await service_client.get('/now')
    assert response.status == 200
    assert response.json() == {'now': '2019-12-31T11:22:33+00:00'}

    # Change mocked time and sync state
    mocked_time.sleep(671)
    await service_client.update_server_state()

    response = await service_client.get('/now')
    assert response.status == 200
    assert response.json() == {'now': '2019-12-31T11:33:44+00:00'}
    # /// [mocked_time]


@pytest.mark.now('2019-12-31T10:00:00+0545')
async def test_now_timezone_custom(service_client, mocked_time):
    response = await service_client.get('/now')
    assert response.status == 200
    assert response.json() == {'now': '2019-12-31T04:15:00+00:00'}

    mocked_time.set(dateutil.parser.parse('2019-12-31T10:00:00+0400'))

    response = await service_client.get('/now')
    assert response.status == 200
    assert response.json() == {'now': '2019-12-31T06:00:00+00:00'}

import pytest

from testsuite import utils


@pytest.mark.now('2019-12-31T11:22:33Z')
async def test_now(test_service_client, mocked_time):
    response = await test_service_client.post(
        '/tests/control',
        json={'mock_now': utils.timestring(mocked_time.now())},
    )
    assert response.status == 200

    response = await test_service_client.get('/now')
    assert response.status == 200
    assert response.json() == {'now': '2019-12-31T11:22:33+00:00'}

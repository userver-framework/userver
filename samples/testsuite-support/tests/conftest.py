import pytest

pytest_plugins = ['pytest_userver.plugins.log_capture']


@pytest.fixture
def tests_control(test_service_client):
    async def do_tests_control(body):
        response = await test_service_client.post('/tests/control', json=body)
        assert response.status_code == 200

    return do_tests_control

import requests


# /// [Functional test]
async def test_ping(service_client, service_port):
    response = requests.get(
        f'https://localhost:{service_port}/hello', verify=False,
    )
    assert response.status_code == 200
    assert response.text == 'Hello world!\n'
    # /// [Functional test]

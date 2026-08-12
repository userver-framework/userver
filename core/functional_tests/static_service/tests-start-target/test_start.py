"""
Makes sure that the service started via the `start-*` cmake target serves
requests, performing an analogous request to the neighboring
`tests-serve-from-root/` tests.
"""


async def test_static_via_start_target(service_client):
    response = await service_client.get('/index.html')
    assert response.status == 200
    assert response.headers['Content-Type'] == 'text/html'

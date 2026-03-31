async def test_trigger_periodic(service_client):
    # PeriodicTask registered with `utils::StartPeriodicTask` doesn't actually
    # start in testsuite
    response = await service_client.get('/ticks')
    assert response.status == 200
    assert response.json() == 0

    # Explicitly trigger periodic as if it is the time to run
    await service_client.run_periodic('tick')

    response = await service_client.get('/ticks')
    assert response.status == 200
    assert response.json() == 1

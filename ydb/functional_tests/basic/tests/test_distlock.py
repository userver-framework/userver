async def test_works(service_client, testpoint):
    @testpoint('dist-lock-foo')
    async def foo_testpoint(_data):
        pass

    @testpoint('dist-lock-bar')
    async def bar_testpoint(_data):
        pass

    # /// [run]
    await service_client.run_distlock_task('sample-dist-lock')
    # /// [run]
    assert foo_testpoint.times_called == 1
    assert bar_testpoint.times_called == 1

async def test_log_action(service_client, pgsql, mockserver):
    @mockserver.handler('/v1/action')
    def _mock(request):
        assert request.get_data() == b'test_1', f'Actual data is {request.get_data()}'
        return mockserver.make_response()

    response = await service_client.post('/log?action=test_1')
    assert response.status == 200

    # There is a 'postgresql/schemas/db_1.sql' file, so the database is 'db_1'
    cursor = pgsql['db_1'].cursor()
    cursor.execute('SELECT action FROM events_table WHERE id=1')
    result = cursor.fetchall()
    assert len(result) == 1
    assert result[0][0] == 'test_1'

import aiohttp


async def send_to_testpoint(mockserver, statement_type, statement_name):
    url = mockserver.url('/testpoint')
    async with aiohttp.ClientSession() as session:
        async with session.post(url, json={'name': statement_type, 'data': {'name': statement_name}}) as response:
            data = await response.json()

    return data


def test_sql_coverage(sql_coverage, on_uncovered):
    sql_coverage.validate(on_uncovered)


async def test_query1(mockserver):
    response = await send_to_testpoint(mockserver, 'sql_statement', 'query1')
    assert response['handled']


async def test_query2(mockserver):
    response = await send_to_testpoint(mockserver, 'sql_statement', 'query2')
    assert response['handled']


async def test_query3(mockserver):
    response = await send_to_testpoint(mockserver, 'sql_statement', 'query3')
    assert response['handled']

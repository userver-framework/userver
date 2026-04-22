import aiohttp.web


async def test_websocket_client(service_client, mockserver):
    """Test WebSocket client connecting to echo server"""

    @mockserver.aiohttp_handler('/chat')
    async def _mock_websocket(request):
        ws = aiohttp.web.WebSocketResponse()
        await ws.prepare(request)

        async for msg in ws:
            if msg.type == aiohttp.WSMsgType.TEXT:
                await ws.send_str(msg.data)

        return ws

    response = await service_client.get(
        '/ws-client',
        params={
            'url': mockserver.ws_url('/chat'),
            'message': 'test message',
        },
    )
    assert response.status == 200
    assert response.text == 'test message'

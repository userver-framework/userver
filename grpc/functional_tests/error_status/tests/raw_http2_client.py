import asyncio
import dataclasses
import logging
import struct
import urllib.parse

import h2.config
import h2.connection
import h2.events


@dataclasses.dataclass
class GrpcResponse:
    status: int
    message: str
    data: bytes
    headers: dict[str, str] = dataclasses.field(default_factory=dict)
    trailers: dict[str, str] = dataclasses.field(default_factory=dict)


class RawGrpcClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port

    async def call(
        self,
        service: str,
        method: str,
        request_bytes: bytes = b'',
        *,
        metadata: dict[str, str] | None = None,
        timeout: float = 5.0,
    ) -> GrpcResponse:
        logging.debug(f'Starting call {service}.{method}')

        return await asyncio.wait_for(
            self._call_impl(service, method, request_bytes, metadata or {}),
            timeout=timeout,
        )

    async def _call_impl(
        self,
        service: str,
        method: str,
        request_bytes: bytes,
        metadata: dict[str, str],
    ) -> GrpcResponse:
        logging.debug(f'Connecting to {self.host}:{self.port}')
        reader, writer = await asyncio.open_connection(self.host, self.port)
        logging.debug(f'Connected to {self.host}:{self.port}')

        try:
            config = h2.config.H2Configuration(client_side=True)
            conn = h2.connection.H2Connection(config=config)
            conn.initiate_connection()
            writer.write(conn.data_to_send())

            logging.debug('Writing initial connection')
            await writer.drain()
            logging.debug('Initial connection written')

            # https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md
            headers = [
                (':method', 'POST'),
                (':path', f'/{service}/{method}'),
                (':scheme', 'http'),
                (':authority', f'{self.host}:{self.port}'),
                ('content-type', 'application/grpc+proto'),
                ('te', 'trailers'),
            ]
            for k, v in metadata.items():
                headers.append((k.lower(), v))

            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, headers)

            frame = struct.pack('>?I', False, len(request_bytes)) + request_bytes
            conn.send_data(stream_id, frame, end_stream=True)
            writer.write(conn.data_to_send())

            logging.debug('Writing request')
            await writer.drain()
            logging.debug('Request written')

            response_data = bytearray()
            response_headers: dict[str, str] = {}
            response_trailers: dict[str, str] = {}
            raw_status: int | None = None
            status_message: str | None = None

            while True:
                logging.debug('Waiting for data')
                data = await reader.read(65535)
                if not data:
                    logging.debug('No data')
                    break
                logging.debug('Got data')

                for event in conn.receive_data(data):
                    if isinstance(event, h2.events.ResponseReceived):
                        logging.debug(f'Got response event: {event.headers}')
                        response_headers = _decode_headers(event.headers)
                        # grpc-status and grpc-message can come in headers (Trailers-Only response)
                        if 'grpc-status' in response_headers:
                            raw_status = int(response_headers['grpc-status'])
                            status_message = urllib.parse.unquote(response_headers.get('grpc-message', ''))
                            logging.debug(f'Got grpc-status from headers: {raw_status}')

                    elif isinstance(event, h2.events.DataReceived):
                        logging.debug(f'Got data event: {event.data}')
                        response_data.extend(event.data)
                        conn.acknowledge_received_data(event.flow_controlled_length, event.stream_id)

                    elif isinstance(event, h2.events.TrailersReceived):
                        logging.debug(f'Got trailers event: {event.headers}')
                        response_trailers = _decode_headers(event.headers)
                        if 'grpc-status' in response_trailers:
                            raw_status = int(response_trailers['grpc-status'])
                            status_message = urllib.parse.unquote(response_trailers.get('grpc-message', ''))
                            logging.debug(f'Got grpc-status from trailers: {raw_status}')

                    elif isinstance(event, h2.events.StreamEnded):
                        logging.debug('Got stream ended event')
                        return GrpcResponse(
                            status=raw_status if raw_status is not None else 0,
                            message=status_message if status_message is not None else '',
                            data=_parse_frames(bytes(response_data)),
                            headers=response_headers,
                            trailers=response_trailers,
                        )

                if to_send := conn.data_to_send():
                    writer.write(to_send)
                    await writer.drain()

            return GrpcResponse(
                status=raw_status if raw_status is not None else 0,
                message=status_message if status_message is not None else '',
                data=_parse_frames(bytes(response_data)),
                headers=response_headers,
                trailers=response_trailers,
            )

        finally:
            logging.debug(f'Closing connection to {self.host}:{self.port}')
            writer.close()
            await writer.wait_closed()


def _decode_headers(headers: list) -> dict[str, str]:
    return {
        (k.decode() if isinstance(k, bytes) else k): (v.decode() if isinstance(v, bytes) else v) for k, v in headers
    }


def _parse_frames(data: bytes) -> bytes:
    result = bytearray()
    offset = 0
    while offset + 5 <= len(data):
        _, length = struct.unpack('>?I', data[offset : offset + 5])
        offset += 5
        if offset + length > len(data):
            break
        result.extend(data[offset : offset + length])
        offset += length
    return bytes(result)

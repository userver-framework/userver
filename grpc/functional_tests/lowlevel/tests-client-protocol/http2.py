import asyncio.streams as streams
import dataclasses
from typing import Callable
from typing import List
from typing import Optional
from typing import Tuple

import h2.config as config
import h2.connection as connection
import h2.errors as errors
import h2.events as events


class Frame:
    "Base class for all frames"


@dataclasses.dataclass(frozen=True)
class HeadersFrame(Frame):
    headers: List[Tuple[str, str]]
    end_stream: bool = False


@dataclasses.dataclass(frozen=True)
class DataFrame(Frame):
    data: bytes


@dataclasses.dataclass(frozen=True)
class RstStreamFrame(Frame):
    error_code: errors.ErrorCodes


@dataclasses.dataclass
class GrpcServer:
    response_factory: Optional[Callable[[], List[Frame]]] = None
    endpoint: Optional[str] = None

    async def __call__(self, reader: streams.StreamReader, writer: streams.StreamWriter) -> None:
        conn = connection.H2Connection(config=config.H2Configuration(client_side=False))
        conn.initiate_connection()
        writer.write(conn.data_to_send())

        try:
            while True:
                data = await reader.read(65536)
                if not data:
                    break

                for event in conn.receive_data(data):
                    if isinstance(event, events.RequestReceived):
                        self._handle_request(conn, event)
                    elif isinstance(event, events.DataReceived):
                        conn.acknowledge_received_data(event.flow_controlled_length, event.stream_id)

                writer.write(conn.data_to_send())
                await writer.drain()
        except ConnectionResetError:
            pass
        finally:
            writer.close()
            await writer.wait_closed()

    def _handle_request(self, conn: connection.H2Connection, event: events.RequestReceived):
        stream_id = event.stream_id
        assert stream_id is not None

        if not self.response_factory:
            return

        for frame in self.response_factory():
            if isinstance(frame, HeadersFrame):
                conn.send_headers(stream_id, headers=frame.headers, end_stream=frame.end_stream)
            elif isinstance(frame, DataFrame):
                conn.send_data(stream_id, data=frame.data)
            elif isinstance(frame, RstStreamFrame):
                conn.reset_stream(stream_id, error_code=frame.error_code)
            else:
                assert False

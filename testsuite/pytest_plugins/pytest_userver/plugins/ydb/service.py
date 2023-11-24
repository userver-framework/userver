import dataclasses
import os
import pathlib
import typing

from testsuite.environment import service
from testsuite.environment import utils


DEFAULT_HOST = 'localhost'
DEFAULT_GRPC_TLS_PORT = 2135
DEFAULT_GRPC_PORT = 2136
DEFAULT_MON_PORT = 8765
DEFAULT_DATABASE = 'local'
DEFAULT_CONTAINER_NAME = 'ydb-local-testsuite'
DEFAULT_DOCKER_IMAGE = 'cr.yandex/yc/yandex-docker-local-ydb:latest'

PLUGIN_DIR = pathlib.Path(__file__).parent
SCRIPTS_DIR = PLUGIN_DIR.joinpath('scripts')


@dataclasses.dataclass(frozen=True)
class ServiceSettings:
    host: str
    grpc_port: int
    mon_port: int
    ic_port: int
    database: str


def create_ydb_service(
        service_name: str,
        working_dir: str,
        settings: typing.Optional[ServiceSettings] = None,
        env: typing.Optional[typing.Dict[str, str]] = None,
):
    if settings is None:
        settings = get_service_settings()
    return service.ScriptService(
        service_name=service_name,
        script_path=str(SCRIPTS_DIR.joinpath('service-ydb')),
        working_dir=working_dir,
        environment={
            'YDB_TMPDIR': working_dir,
            'YDB_SCRIPTS_DIR': str(SCRIPTS_DIR),
            'YDB_HOSTNAME': settings.host,
            'YDB_GRPC_PORT': str(settings.grpc_port),
            'YDB_GRPC_TLS_PORT': str(DEFAULT_GRPC_TLS_PORT),
            'YDB_MON_PORT': str(settings.mon_port),
            'YDB_CONTAINER_NAME': DEFAULT_CONTAINER_NAME,
            'YDB_DOCKER_IMAGE': DEFAULT_DOCKER_IMAGE,
            **(env or {}),
        },
        check_ports=[settings.grpc_port, settings.mon_port],
    )


def get_service_settings():
    return ServiceSettings(
        host=os.getenv('TESTSUITE_YDB_HOST', DEFAULT_HOST),
        grpc_port=utils.getenv_int(
            'TESTSUITE_YDB_GRPC_PORT', DEFAULT_GRPC_PORT,
        ),
        mon_port=utils.getenv_int('TESTSUITE_YDB_MON_PORT', DEFAULT_MON_PORT),
        ic_port=utils.getenv_int('TESTSUITE_YDB_IC_PORT', 0),
        database=os.getenv('TESTSUITE_YDB_DATABASE', DEFAULT_DATABASE),
    )

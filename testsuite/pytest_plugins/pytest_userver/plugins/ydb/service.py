import dataclasses
import os
import pathlib
import typing

from testsuite.environment import service
from testsuite.environment import utils


DEFAULT_HOST = 'localhost'
DEFAULT_GRPC_PORT = 19000
DEFAULT_MON_PORT = 19001
DEFAULT_IC_PORT = 19002
DEFAULT_DATABASE = 'local'

DEFAULT_PDISK_SIZE = 8 * 1024 * 1024 * 1024  # 8gb
DEFAULT_PDISK_CHUNK_SIZE = 32 * 1024 * 1024  # 32mb
DEFAULT_PDISK_SECTOR_SIZE = 2 * 1024  # 4kb
DEFAULT_PDISK_MASTER_KEY = 2748

PLUGIN_DIR = pathlib.Path(__file__).parent
SCRIPTS_DIR = PLUGIN_DIR.joinpath('scripts')


@dataclasses.dataclass(frozen=True)
class ServiceSettings:
    host: str
    grpc_port: int
    mon_port: int
    ic_port: int
    database: str


def create_service(
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
            'YDB_HOST': settings.host,
            'YDB_GRPC_PORT': str(settings.grpc_port),
            'YDB_MON_PORT': str(settings.mon_port),
            'YDB_IC_PORT': str(settings.ic_port),
            'YDB_PDISK_SIZE': str(DEFAULT_PDISK_SIZE),
            'YDB_PDISK_CHUNK_SIZE': str(DEFAULT_PDISK_CHUNK_SIZE),
            'YDB_PDISK_SECTOR_SIZE': str(DEFAULT_PDISK_SECTOR_SIZE),
            'YDB_PDISK_MASTER_KEY': str(DEFAULT_PDISK_MASTER_KEY),
            **(env or {}),
        },
        check_ports=[settings.grpc_port, settings.mon_port, settings.ic_port],
    )


def get_service_settings():
    return ServiceSettings(
        host=os.getenv('TESTSUITE_YDB_HOST', DEFAULT_HOST),
        grpc_port=utils.getenv_int(
            'TESTSUITE_YDB_GRPC_PORT', DEFAULT_GRPC_PORT,
        ),
        mon_port=utils.getenv_int('TESTSUITE_YDB_MON_PORT', DEFAULT_MON_PORT),
        ic_port=utils.getenv_int('TESTSUITE_YDB_IC_PORT', DEFAULT_IC_PORT),
        database=os.getenv('TESTSUITE_YDB_DATABASE', DEFAULT_DATABASE),
    )

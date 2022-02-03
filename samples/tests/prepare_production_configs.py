import os
import pathlib
import shutil

import sample_utils

PRODUCTION_SERVICE_CFG_PATH = pathlib.Path('/tmp/userver/production_service')


if __name__ == '__main__':
    shutil.rmtree(PRODUCTION_SERVICE_CFG_PATH, ignore_errors=True)
    os.makedirs(PRODUCTION_SERVICE_CFG_PATH)
    sample_utils.copy_service_configs(
        'production_service', destination=PRODUCTION_SERVICE_CFG_PATH,
    )

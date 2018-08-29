import argparse
import os.path

from taxi_tests import db
from taxi_tests import json_util

MONGO_ADDRESS = 'localhost:27117'

SETTINGS_OVERRIDE = {
    'RFID_LABELS': {
        'login': 'web',
        'password': 'web',
        'token': (
            'DD FF B9 18 69 B6 5E 94 47 FF DF 2F 7C 2B C1 F6 '
            'FC D2 A1 6F 7E D6 8E 2F 67 8D 34 AA D4 2D DD 0E'
        ),
    },
    'YT_CONFIG': {
        'yt-test': {
            'token': '',
            'prefix': '//home/taxi/',
            'pickling': {
                'enable_tmpfs_archive': False
            },
            'proxy': {
                'url': 'localhost:9999/yt/yt-test'
            },
            'api_version': 'v3'
        },
        'yt-repl': {
            'token': '',
            'prefix': '//home/taxi/',
            'pickling': {
                'enable_tmpfs_archive': False
            },
            'proxy': {
                'url': 'localhost:9999/yt/yt-repl'
            },
            'api_version': 'v3'
        }
    },
    'AFS_STORAGE_SETTINGS': {
          'tvm_client_secret': '',
          'private_key_path': '$tvm_private_key',
          'tvm_login': '',
          'tvm_client_id': '',
          'mds_bucket': '',
          'mds_service_id': ''
    },
    'GPS_STORAGE_SETTINGS': {
          'tvm_client_secret': '',
          'private_key_path': '$tvm_private_key',
          'tvm_login': '',
          'tvm_client_id': '',
          'mds_bucket': '',
          'mds_service_id': ''
    },
    'COMPANION_STORAGE_SETTINGS': {
        'tvm_client_secret': '',
        'private_key_path': '$tvm_private_key',
        'tvm_login': '',
        'tvm_client_id': '',
        'mds_bucket': '',
        'mds_service_id': ''
    },
    "BIOMETRY_STORAGE_SETTINGS": {
        'tvm_client_secret': '',
        'private_key_path': '$tvm_private_key',
        'tvm_login': '',
        'tvm_client_id': '',
        'mds_bucket': '',
        'mds_service_id': ''
    },
    "EXPERIMENTS_STORAGE_SETTINGS": {
        'tvm_client_secret': '',
        'private_key_path': '$tvm_private_key',
        'tvm_login': '',
        'tvm_client_id': '',
        'mds_bucket': '',
        'mds_service_id': ''
    }
}

REDIS_DEFAULT_BASE = {
  'command_control': {
    'max_retries': 3,
    'timeout_all_ms': 1000,
    'timeout_single_ms': 1000
  },
  'password': '',
  'sentinels': [
    {
      'host': 'localhost',
      'port': 26379
    }
  ],
  'shards': [
    {
      'name': 'test_master'
    }
  ]
}


def get_redis_shards(shards):
    settings = REDIS_DEFAULT_BASE.copy()
    settings['shards'] = [{'name': shard} for shard in shards]
    return settings

REDIS_OVERRIDE = {
  'redis_settings': {
    'taximeter-base': REDIS_DEFAULT_BASE,
    'taximeter-hour': REDIS_DEFAULT_BASE,
    'taximeter-temp': REDIS_DEFAULT_BASE,
    'taxi-chat': REDIS_DEFAULT_BASE,
    'taxi-tmp': REDIS_DEFAULT_BASE,
    'taxi-passing': REDIS_DEFAULT_BASE,
    'taxi-experiments': REDIS_DEFAULT_BASE,
    'taxi-geotracks': REDIS_DEFAULT_BASE,
    'taxi-user-tracker': REDIS_DEFAULT_BASE,
    'taxi-surge': REDIS_DEFAULT_BASE,
    'taxi-promotions': REDIS_DEFAULT_BASE,
    'taxi-pool': REDIS_DEFAULT_BASE,
    'taxi-shards-all': get_redis_shards(['test_master', 'test_master2']),
    'taxi-shards-1': get_redis_shards(['test_master']),
    'taxi-shards-2': get_redis_shards(['test_master2']),
  }
}

POSTGRESQL_SHARD_CONFIG = {
    'shard_number': 0,
    'master': 'host=/tmp/testsuite-postgresql user=testsuite dbname=taximeter',
}

POSTGRESQL_OVERRIDE = {
    'postgresql_settings': {
        'composite_tables_count': 1,
        'databases': {
            'orders': [POSTGRESQL_SHARD_CONFIG],
            'payments': [POSTGRESQL_SHARD_CONFIG],
            'misc': [POSTGRESQL_SHARD_CONFIG],
        }
    }
}


def substitute_vars(obj, **kwargs):
    return _substitute_dict(obj, kwargs)


def _substitute_list(seq, options):
    return [_substitute_value(value, options) for value in seq]


def _substitute_value(value, options):
    if isinstance(value, basestring):
        if value.startswith('$'):
            value = options[value[1:]]
    elif isinstance(value, dict):
        value = _substitute_dict(value, options)
    elif isinstance(value, list):
        value = _substitute_list(value, options)
    return value


def _substitute_dict(dct, options):
    return {key: _substitute_value(value, options)
            for key, value in dct.items()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--testsuite-build-dir', help='Build directory path', required=True
    )
    parser.add_argument(
        '--testsuite-dir', help='Testsuite directory path', required=True
    )
    parser.add_argument('input', help='Source secdist file')
    parser.add_argument('output', help='Patched secdist file')
    parser.add_argument('db_settings', help='Source db_settings file')
    args = parser.parse_args()

    with open(args.input) as fp:
        data = json_util.loads(fp.read())

    db_collections = db.get_db_collections(args.db_settings)
    mongo_settings = {
        'stq': {'uri': 'mongodb://%s/%s' % (MONGO_ADDRESS, 'dbstq')}
    }
    for name, (dbalias, dbname, colname) in db_collections.items():
        mongo_settings[dbalias] = {
            'uri': 'mongodb://%s/%s' % (MONGO_ADDRESS, dbname)
        }
    data['mongo_settings'] = mongo_settings
    data['settings_override'].update(SETTINGS_OVERRIDE)
    data.update(REDIS_OVERRIDE)
    data.update(POSTGRESQL_OVERRIDE)

    data = substitute_vars(
        data,
        tvm_private_key=os.path.join(
            os.path.join(args.testsuite_dir, 'configs/tvm_private_key')),
    )

    data = json_util.dumps(data)
    with open(args.output, 'w') as fp:
        fp.write(data)
        fp.write('\n')


if __name__ == '__main__':
    main()

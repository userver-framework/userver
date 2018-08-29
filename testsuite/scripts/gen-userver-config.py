from __future__ import print_function

import argparse
import json
import sys

COMPONENTS_TO_SKIP = ('logging', 'secdist', 'taxi-config',)
MONGO_ADDRESS = 'localhost:27117'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--userver-config',
        help='Path to userver base config file.',
        required=True,
    )
    parser.add_argument(
        '--userver-config-vars',
        help='Path to userver config vars file.',
        required=True,
    )
    parser.add_argument(
        '--userver-fallbacks-config',
        help='Path to userver fallbacks file.',
        required=True,
    )
    parser.add_argument(
        '--secdist',
        help='Path to secdist file.',
        required=True,
    )
    parser.add_argument(
        '-o', '--output',
        help='Path to output file.',
    )
    args = parser.parse_args()

    with open(args.userver_config) as fp:
        userver_config = json.load(fp)

    userver_config['config_vars'] = args.userver_config_vars

    components = []
    for component in userver_config['components_manager']['components']:
        if component['name'] in COMPONENTS_TO_SKIP:
            continue
        components.append(component)
    components.append({
        'name': 'logging',
        'loggers': {}
    })
    components.append({
        'name': 'secdist',
        'config': args.secdist,
    })
    components.append({
        'name': 'taxi-config',
        'fallback_path': args.userver_fallbacks_config,
        'full_update_interval_ms': 60000,
        'update_interval_ms': 5000
    })

    userver_config['components_manager']['components'] = components

    output_data = json.dumps(
        userver_config, ensure_ascii=False, sort_keys=True,
        indent=2, separators=(',', ': ')
    )
    if args.output is None:
        sys.stdout.write(output_data)
    else:
        with open(args.output, 'w') as fp:
            fp.write(output_data)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
# pylint: disable=invalid-name

import argparse
import json
import os
import subprocess
import sys
import typing

import requests
import yaml

CONFIG_BASEPATH = '/etc/yandex/taxi/'


class Client:
    def __init__(self, args):
        self.args = args
        self.read_config_yaml(self.args.config)
        self.monitor_url = self.read_monitor_url()
        self.config_yaml: typing.Any = None
        self.config_vars: typing.Any = None

    def client_send(
            self,
            path: str,
            method: str,
            params: typing.Optional[typing.Dict[str, str]] = None,
            body: typing.Optional[str] = None,
    ) -> str:
        call = getattr(requests, method)
        response = call(self.monitor_url + path, params=params, data=body)
        response.raise_for_status()
        return response.text

    def dns_reload_hosts(self) -> None:
        self.client_send(path='/service/dnsclient/reload_hosts', method='post')

    def dns_flush_cache(self) -> None:
        self.client_send(
            path='/service/dnsclient/flush_cache',
            method='post',
            params={'name': self.args.dns_name},
        )

    def dns_flush_cache_full(self) -> None:
        self.client_send(
            path='/service/dnsclient/flush_cache_full', method='post',
        )

    def log_set_level(self) -> None:
        self.client_send(
            path=f'/service/log-level/{self.args.level}', method='put',
        )

    def log_get_level(self) -> None:
        data = self.client_send(path='/service/log-level/', method='get')
        level = json.loads(data)['current-log-level']
        print(level)

    def on_logrotate(self) -> None:
        self.client_send(path='/service/on-log-rotate', method='post')

    def log_dynamic_debug_list(self) -> None:
        data = self.client_send(path='/log/dynamic-debug', method='get')
        print(data, end='')

    def log_dynamic_debug_force_on(self) -> None:
        self.client_send(
            path='/log/dynamic-debug',
            method='put',
            params={'location': self.args.location},
            body='1',
        )

    def log_dynamic_debug_force_off(self) -> None:
        self.client_send(
            path='/log/dynamic-debug',
            method='put',
            params={'location': self.args.location},
            body='-1',
        )

    def log_dynamic_debug_set_default(self) -> None:
        self.client_send(
            path='/log/dynamic-debug',
            method='delete',
            params={'location': self.args.location},
        )

    def stats(self) -> None:
        data = self.client_send(
            path='/', method='get', params={'format': 'pretty'},
        )
        print(data, end='')

    def inspect_requests(self) -> None:
        data = self.client_send(path='/service/inspect-requests', method='get')
        print(data)

    def access_top(self) -> None:
        components = self.config_yaml['components_manager']['components']
        logger_path = components['logging']['loggers']['default']['file_path']
        if logger_path.startswith('$'):
            logger_path = self.config_vars[logger_path[1:]]

        subprocess.check_call(
            ['access-top', '--service_log_filepath', logger_path],
        )

    def read_config_yaml(self, config_yaml: str) -> None:
        try:
            with open(config_yaml, 'r') as ifile:
                self.config_yaml = yaml.safe_load(ifile)
        except FileNotFoundError:
            print(
                'File "config.yaml" not found, maybe you forgot '
                'to pass --config?',
                file=sys.stderr,
            )
            sys.exit(1)

        config_vars_path = self.config_yaml['config_vars']
        with open(config_vars_path, 'r') as ifile:
            self.config_vars = yaml.load(ifile)

    def config_yaml_read(self, path: typing.List[str]) -> str:
        data = self.config_yaml
        while len(path) > 1:
            data = data[path[0]]
            path = path[1:]

        value: str = str(data[path[0]])
        if not value.startswith('$'):
            return value

        try:
            return self.config_vars[value[1:]]
        except KeyError:
            return data[path[0] + '#fallback']

    def read_monitor_url(self) -> str:
        port = self.config_yaml_read(
            [
                'components_manager',
                'components',
                'server',
                'listener-monitor',
                'port',
            ],
        )
        return f'http://localhost:{port}'


def guess_config_yaml() -> str:
    try:
        argv0 = sys.argv[0]

        basename = os.path.basename(argv0)
        if basename.endswith('-ctl'):
            basename = basename[: -len('-ctl')]
        elif basename.endswith('-ctl.py'):
            basename = basename[: -len('-ctl.py')]

        config_yaml = os.path.join(CONFIG_BASEPATH, basename, 'config.yaml')
        return config_yaml
    except BaseException:
        return ''


def parse_args(args: typing.List[str]):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--config', help='config.yaml', default=guess_config_yaml(), type=str,
    )

    subparsers = parser.add_subparsers()

    parser_dns = subparsers.add_parser('dnsclient', help='DNS client')
    subparsers_dns = parser_dns.add_subparsers()

    parser_reload_hosts = subparsers_dns.add_parser(
        'reload-hosts', help='Reload /etc/hosts file',
    )
    parser_reload_hosts.set_defaults(func=Client.dns_reload_hosts)

    parser_flush_cache = subparsers_dns.add_parser(
        'flush-cache', help='Flush DNS cache',
    )
    parser_flush_cache.add_argument('dns_name')
    parser_flush_cache.set_defaults(func=Client.dns_flush_cache)

    parser_flush_cache_full = subparsers_dns.add_parser(
        'flush-cache-full', help='Flush DNS cache full',
    )
    parser_flush_cache_full.set_defaults(func=Client.dns_flush_cache_full)

    parser_log_dynamic_debug = subparsers.add_parser(
        'log-dynamic-debug', help='Dynamic debug log level',
    )
    subparsers_log_dynamic = parser_log_dynamic_debug.add_subparsers()

    parser_log_dynamic_debug_list = subparsers_log_dynamic.add_parser(
        'list', help='list log entries',
    )
    parser_log_dynamic_debug_list.set_defaults(
        func=Client.log_dynamic_debug_list,
    )

    parser_log_dynamic_debug_force_on = subparsers_log_dynamic.add_parser(
        'force-on', help='force enable log entry',
    )
    parser_log_dynamic_debug_force_on.add_argument('location')
    parser_log_dynamic_debug_force_on.set_defaults(
        func=Client.log_dynamic_debug_force_on,
    )

    parser_log_dynamic_debug_force_off = subparsers_log_dynamic.add_parser(
        'force-off', help='force disable log entry',
    )
    parser_log_dynamic_debug_force_off.add_argument('location')
    parser_log_dynamic_debug_force_off.set_defaults(
        func=Client.log_dynamic_debug_force_off,
    )

    parser_log_dynamic_debug_set_default = subparsers_log_dynamic.add_parser(
        'set-default', help='drop dynamic debug log entry',
    )
    parser_log_dynamic_debug_set_default.add_argument('location')
    parser_log_dynamic_debug_set_default.set_defaults(
        func=Client.log_dynamic_debug_set_default,
    )

    parser_log = subparsers.add_parser('log-level', help='Logs operations')
    subparsers_log = parser_log.add_subparsers()

    parser_log_set_level = subparsers_log.add_parser(
        'set', help='Set log level',
    )
    parser_log_set_level.add_argument(
        'level',
        choices=[
            'default',
            'trace',
            'debug',
            'info',
            'warning',
            'error',
            'none',
        ],
        help='Minimum log item level to log',
    )
    parser_log_set_level.set_defaults(func=Client.log_set_level)

    parser_log_get_level = subparsers_log.add_parser(
        'get', help='Get log level',
    )
    parser_log_get_level.set_defaults(func=Client.log_get_level)

    parser_logrotate = subparsers.add_parser(
        'on-logrotate', help='Process logrotate post-actions',
    )
    parser_logrotate.set_defaults(func=Client.on_logrotate)

    parser_stats = subparsers.add_parser('stats')
    parser_stats.set_defaults(func=Client.stats)

    parser_inspect_requests = subparsers.add_parser('inspect-requests')
    parser_inspect_requests.set_defaults(func=Client.inspect_requests)

    parser_access_top = subparsers.add_parser('access-top')
    parser_access_top.set_defaults(func=Client.access_top)

    opts = parser.parse_args(args)
    return opts


def run(argv: typing.List[str]) -> None:
    args = parse_args(argv)
    if not hasattr(args, 'func'):
        parse_args(argv + ['--help'])
        return

    args.func(Client(args))


def main() -> None:
    run(sys.argv[1:])


if __name__ == '__main__':
    main()

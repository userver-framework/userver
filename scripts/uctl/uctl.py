#!/usr/bin/env python3

import argparse
import asyncio
import json
import os
import subprocess
import sys
import typing

import aiohttp
import yaml


CONFIG_BASEPATH = '/etc/yandex/taxi/'


class Client:
    def __init__(self, args):
        self.args = args
        self.read_config_yaml(self.args.config)
        self.monitor_url = self.read_monitor_url()
        self.session = aiohttp.ClientSession()

    async def client_send(
            self,
            path: str,
            method: str,
            params: typing.Optional[typing.Dict[str, str]] = None,
    ) -> str:
        call = getattr(self.session, method)
        response = await call(self.monitor_url + path, params=params)
        response.raise_for_status()
        return await response.text()

    async def dns_reload_hosts(self) -> None:
        await self.client_send(
            path='/service/dnsclient/reload_hosts', method='post',
        )

    async def dns_flush_cache(self) -> None:
        await self.client_send(
            path='/service/dnsclient/flush_cache',
            method='post',
            params={'name': self.args.dns_name},
        )

    async def dns_flush_cache_full(self) -> None:
        await self.client_send(
            path='/service/dnsclient/flush_cache_full', method='post',
        )

    async def log_set_level(self) -> None:
        await self.client_send(
            path=f'/service/log-level/{self.args.level}', method='put',
        )

    async def log_get_level(self) -> None:
        data = await self.client_send(path='/service/log-level/', method='get')
        level = json.loads(data)['current-log-level']
        print(level)

    async def on_logrotate(self) -> None:
        await self.client_send(path='/service/on-log-rotate', method='post')

    async def log_dynamic_debug_list(self) -> None:
        data = await self.client_send(path='/log/dynamic-debug', method='get')
        print(data, end='')

    async def stats(self) -> None:
        data = await self.client_send(
            path='/', method='get', params={'format': 'pretty'},
        )
        print(data, end='')

    async def inspect_requests(self) -> None:
        data = await self.client_send(
            path='/service/inspect-requests', method='get',
        )
        print(data)

    async def access_top(self) -> None:
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
    parser_log_dynamic_debug.set_defaults(func=Client.log_dynamic_debug_list)

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


async def run(argv: typing.List[str]) -> None:
    args = parse_args(argv)
    if not hasattr(args, 'func'):
        parse_args(argv + ['--help'])
        return

    await args.func(Client(args))


def main() -> None:
    asyncio.run(run(sys.argv[1:]))


if __name__ == '__main__':
    main()

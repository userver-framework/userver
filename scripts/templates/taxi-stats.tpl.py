#!/usr/bin/python

import argparse
import numbers
import requests


def flatten_json(prefix, obj):
    result = {}
    for key in obj:
        if prefix:
            prefixed_key = prefix + '.' + key
        else:
            prefixed_key = key
        value = obj[key]
        if isinstance(value, dict):
            values = flatten_json(prefixed_key, value)
            result.update(values)
        elif isinstance(value, numbers.Number):
            result[prefixed_key] = value
    return result


def handle_options():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--hostname', help='Hostname and monitor port of %NAME% service',
        default='[::1]:1188', type=str)
    parser.add_argument(
        '--url', help='statistics url', default='/',
        type=str)
    parser.add_argument(
        '--timeout', help='HTTP timeout (in seconds)', default=5, type=float)
    opts = parser.parse_args()
    return opts


def main():
    opts = handle_options()

    headers = {'host': '%HOSTNAME%'}
    url = 'http://' + opts.hostname + opts.url
    r = requests.get(url, timeout=opts.timeout,
                     headers=headers)
    r.raise_for_status()

    j = r.json()
    fl = flatten_json('', j)
    for k in fl:
        print('{} {}'.format(k, fl[k]))

if __name__ == '__main__':
    main()

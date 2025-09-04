#!/usr/bin/env python3

import sys


def filter_doxygen_log(lines):
    skipping = False

    for line in lines:
        if skipping:
            if line.startswith(' ') or line.strip() == 'Possible candidates:':
                continue
            skipping = False
        if 'matching class member found for' in line:
            skipping = True
            continue
        yield line


def main():
    for line in filter_doxygen_log(sys.stdin):
        print(line, end='')
        sys.stdout.flush()


if __name__ == '__main__':
    main()

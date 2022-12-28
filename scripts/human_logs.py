#!/usr/bin/env python

import argparse
import itertools
import sys


# Info about colors https://misc.flogisoft.com/bash/tip_colors_and_formatting
class Colors:
    BLACK = '\033[30m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    GRAY = '\033[37m'
    DARK_GRAY = '\033[37m'
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    DEFAULT = '\033[0m'
    DEFAULT_BG = '\033[49m'
    BG_BLACK = '\033[40m'

    # No bright red color, no close colors, no too dark colors
    __NICE_COLORS = [
        '\033[38;5;{}m'.format(x)
        for x in itertools.chain(
            range(1, 7),
            range(10, 15),
            range(38, 51, 2),
            range(75, 87, 2),
            range(112, 123, 3),
            range(128, 195, 3),
            range(200, 231, 3),
        )
    ]

    @staticmethod
    def colorize(value, active_color=DEFAULT):
        ind = hash(value) % len(Colors.__NICE_COLORS)
        return Colors.__NICE_COLORS[ind] + str(value) + active_color


class HumanLogs:
    LOG_LEVELS = ('TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL')
    LOG_LEVEL_COLORS = (
        Colors.DARK_GRAY,
        Colors.GRAY,
        Colors.GREEN,
        Colors.YELLOW,
        Colors.RED,
        Colors.BRIGHT_RED,
    )

    def __init__(
            self, highlights, ignores, filename, no_file_stores, verbosity,
    ):
        self.i = 0
        self.highlights = highlights
        self.ignores = ignores
        self.verbosity_indx = HumanLogs.LOG_LEVELS.index(verbosity)

        self.full_logs = None
        if not no_file_stores:
            self.full_logs_path = filename
            # We truncate file, so that line number from this program
            # output match line number in full_logs_path.
            self.full_logs = open(self.full_logs_path, 'w')

    def __output_line(self, line):
        self.i += 1

        if not line.startswith('tskv\t'):
            sys.stdout.write('{:<3} {}'.format(self.i, line))
            sys.stdout.flush()
            return

        line = line[5:]  # Dropping "tskv\t"
        line = line.strip()
        values = {}
        for item in line.split('\t'):
            key, _, value = item.partition('=')
            values[key] = value

        level = values.pop('level', 'CRITICAL')

        text = values.pop('text', '')
        if 'text' in self.ignores:
            text = ''

        text_color = Colors.GREEN
        if level in ('DEBUG', 'TRACE'):
            text_color = Colors.DARK_GRAY

        module = values.pop('module', '?')
        if 'module' in self.ignores:
            module = ''
        else:
            module = '{BLUE}=> {module} '.format(module=module, **vars(Colors))

        for x in self.highlights:
            if x in values:
                values[x] = Colors.colorize(values[x], Colors.GRAY)

        fmt = (
            '{i:<3} {level_color}{level:<5} {text_color}{text} '
            '{module}{gray_color}{remains}{default_color}\n'
        )

        level_indx = HumanLogs.LOG_LEVELS.index(level)

        remains = ''
        if level_indx > self.verbosity_indx:
            remains = (
                x
                for x in values
                if (x not in self.ignores and values[x] != '')
            )
            remains = '\t'.join(x + '=' + values[x] for x in sorted(remains))

        if level == 'WARNING':
            level = 'WARN'

        sys.stdout.write(
            fmt.format(
                i=self.i,
                level_color=HumanLogs.LOG_LEVEL_COLORS[level_indx],
                level=level[:5],
                text_color=text_color,
                text=text,
                module=module,
                remains=remains,
                gray_color=Colors.GRAY,
                default_color=Colors.DEFAULT,
            ),
        )
        sys.stdout.flush()

    def process_file(self, input_file):
        try:

            for line in iter(input_file.readline, ''):
                if self.full_logs is not None:
                    self.full_logs.write(line)
                self.__output_line(line)

        except KeyboardInterrupt:
            pass

        sys.stdout.write(Colors.DEFAULT)

        if self.full_logs is not None:
            fin = '\nFull logs were written into "{logs_path}"\n'.format(
                logs_path=self.full_logs_path,
            )
            sys.stdout.write(fin)
        sys.stdout.flush()


__EXAMPLES = """
Examples:
#1  \033[92mantoshkka@antoshkka-work:~$\033[0m ./human_logs.py -x logs.txt | less -R

    1   \033[37mINFO  \033[32mStopping component \033[34m=> ClearComponent ( component_context_component_info.cpp:41 ) \033[0m
    2   \033[37mINFO  \033[32mStopped component \033[34m=> ClearComponent ( component_context_component_info.cpp:43 ) \033[0m
    3   \033[33mWARN  \033[32mPilorama is stopping \033[34m=> Stop ( pilorama.cpp:133 )  \033[37mcomponent_name=pilorama	coro_id=\033[38;5;14m7FCB4AE08608\033[0m


#2  \033[92mantoshkka@antoshkka-work:~$\033[0m make start-pilorama -j5 2>&1 | ./human_logs.py
    1   make -j4 -C build yandex-taxi-pilorama
    2   make -j4 -C build yandex-taxi-pilorama-gen-config-vars
    <...>
    155 \033[37mINFO  \033[32mStopped component \033[34m=> ClearComponent ( component_context_component_info.cpp:43 ) \033[0m

    Full logs were written into "full_logs.txt"


#3  \033[92mantoshkka@antoshkka-work:~$\033[0m ./human_logs.py -ign module link span_id task_id timestamp timezone trace_id stopwatch_units span_ref_type start_timestamp -x -v DEBUG logs.txt
    <...>
    94  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=statistics-storage	coro_id=\033[38;5;227m7FCB4AE08648\033[37m\033[0m
    95  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=http-client	coro_id=\033[38;5;215m7FCB4AE08680\033[37m\033[0m
    96  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=statistics-storage	coro_id=\033[38;5;227m7FCB4AE08648\033[37m\033[0m
    97  \033[32mINFO  \033[32m \033[37mcomponent_name=statistics-storage	coro_id=\033[38;5;227m7FCB4AE08648\033[37m	stopwatch_name=component_stop	total_time=0.014451\033[0m
    98  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=http-client	coro_id=\033[38;5;215m7FCB4AE08680\033[37m\033[0m
    99  \033[32mINFO  \033[32m \033[37mcomponent_name=http-client	coro_id=\033[38;5;215m7FCB4AE08680\033[37m	stopwatch_name=component_stop	total_time=2.53036\033[0m
    100 \033[32mINFO  \033[32mCall ClearComponent() for component dynamic-config \033[37m\033[0m
    <...>


#4  \033[92mantoshkka@antoshkka-work:~$\033[0m ./human_logs.py -ign module link span_id task_id timestamp timezone trace_id stopwatch_units span_ref_type start_timestamp -v DEBUG -hl component_name -x logs.txt
    <...>
    94  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=\033[38;5;200mstatistics-storage\033[37m	coro_id=7FCB4AE08648\033[0m
    95  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=\033[38;5;161mhttp-client\033[37m	coro_id=7FCB4AE08680\033[0m
    96  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=\033[38;5;200mstatistics-storage\033[37m	coro_id=7FCB4AE08648\033[0m
    97  \033[32mINFO  \033[32m \033[37mcomponent_name=\033[38;5;200mstatistics-storage\033[37m	coro_id=7FCB4AE08648	stopwatch_name=component_stop	total_time=0.014451\033[0m
    98  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=\033[38;5;161mhttp-client\033[37m	coro_id=7FCB4AE08680\033[0m
    99  \033[32mINFO  \033[32m \033[37mcomponent_name=\033[38;5;161mhttp-client\033[37m	coro_id=7FCB4AE08680	stopwatch_name=component_stop	total_time=2.53036\033[0m
    100 \033[32mINFO  \033[32mCall ClearComponent() for component dynamic-config \033[37m\033[0m
    <...>


"""  # noqa: E501


def main():
    parser = argparse.ArgumentParser(
        description='Represents logs nicely',
        epilog=__EXAMPLES,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        '-hl',
        '--highlights',
        nargs='+',
        default=['coro_id'],
        help='Fields to highlight',
    )
    parser.add_argument(
        '-ign',
        '--ignores',
        nargs='+',
        default=['link', 'timestamp', 'timezone'],
        help='Fields to not output',
    )
    parser.add_argument(
        '-f',
        '--file',
        default='full_logs.txt',
        help='File path to store full logs',
    )
    parser.add_argument(
        '-x',
        '--no-file-stores',
        action='store_true',
        help='Do not store full logs into a separate file',
    )
    parser.add_argument(
        '-v',
        '--verbosity',
        choices=HumanLogs.LOG_LEVELS,
        default='WARNING',
        help='Print unimportant fields if log level is'
        ' greater than this value',
    )
    parser.add_argument(
        'input',
        nargs='?',
        default=sys.stdin,
        help='Input file or nothing to read from stdin',
    )

    args = parser.parse_args()
    h_logs = HumanLogs(
        highlights=args.highlights,
        ignores=args.ignores,
        filename=args.file,
        no_file_stores=args.no_file_stores,
        verbosity=args.verbosity,
    )

    if args.input == sys.stdin:
        h_logs.process_file(sys.stdin)
    else:
        with open(args.input) as inp:
            h_logs.process_file(inp)


if __name__ == '__main__':
    main()

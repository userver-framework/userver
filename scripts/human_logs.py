#!/usr/bin/env python

import argparse
import itertools
import sys
import re
import pdb

# Info about colors https://misc.flogisoft.com/bash/tip_colors_and_formatting
class Colors:
    BLACK = '\033[30m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    BRIGHT_BLUE = '\033[94m'
    GRAY = '\033[37m'
    DARK_GRAY = '\033[90m'
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
            self, highlights, ignores, skip_modules, skip_field_patterns, filename, no_file_stores, verbosity, require_fields,
    ):
        self.i = 0
        self.highlights = highlights
        self.ignores = ignores
        self.skip_modules = skip_modules
        self.require_fields = require_fields

        self.skip_field_patterns = {}
        for i in range(0, len(skip_field_patterns), 2):
            key = skip_field_patterns[i]
            value = skip_field_patterns[i + 1]
            self.skip_field_patterns[key] = re.compile(value)

        self.verbosity_indx = HumanLogs.LOG_LEVELS.index(verbosity)

        self.full_logs = None
        if not no_file_stores:
            self.full_logs_path = filename
            # We truncate file, so that line number from this program
            # output match line number in full_logs_path.
            self.full_logs = open(self.full_logs_path, 'w')
    
    def __extract_file_and_line(self, string):
        components = string.split('/')
        if len(components) >= 2:
            return '/'.join(components[-2:])
        return ''

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

        orig_values = values.copy()
        level = values.pop('level', 'CRITICAL')

        text = values.pop('text', '')
        if 'text' in self.ignores:
            text = ''

        text_color = Colors.GREEN
        if level in ('DEBUG', 'TRACE'):
            text_color = Colors.DARK_GRAY

        module = values.pop('module', '?')
        
        pattern = r'^(\w+)\s+\(\s*(.+)\s*\)'
        mod_match = re.match(pattern, module)
        if mod_match:
            module_name = mod_match.group(1) 
        else:
            module_name = module


        if level not in ('ERROR', 'CRITICAL'):
            if module_name in self.skip_modules:
                return
            if self.require_fields:
                has_required_field = any(orig_values.get(field) for field in self.require_fields)
                if not has_required_field:
                    return
            for field, pattern in self.skip_field_patterns.items():
                value = orig_values.get(field)
                if value and pattern.search(value):
                    return
                
        fname = ''
        if mod_match and mod_match.lastindex >= 2:       
            fname = self.__extract_file_and_line( mod_match.group(2))

        if fname and 'fname' in self.ignores:
            fname = ''
        else:
            fname = '{BRIGHT_BLUE}=> {fname} '.format(fname=fname, **vars(Colors))

        if 'module' in self.ignores:
            module = ''
        else:
            module = '{BLUE}=> {module} '.format(module=module, **vars(Colors))
            fname = ''

        for x in self.highlights:
            if x in values:
                values[x] = Colors.colorize(values[x], Colors.GRAY)

        fmt = (
            '{time} {level_color}{level:<5} {text_color}{text} '
            '{module}{fname}{gray_color}{remains}{default_color}\n'
        )

        level_indx = HumanLogs.LOG_LEVELS.index(level)

        remains = ''
        if level_indx > self.verbosity_indx:
            remains = (
                x
                for x in values
                if (x not in self.ignores and values[x] != '' and x != 'timestamp')
            )
            remains = '\t'.join(x + '=' + values[x] for x in sorted(remains))

        if level == 'WARNING':
            level = 'WARN'

        sys.stdout.write(
            fmt.format(
                time=values.get('timestamp'),
                level_color=HumanLogs.LOG_LEVEL_COLORS[level_indx],
                level=level[:5],
                text_color=text_color,
                text=text,
                module=module,
                fname=fname,
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


  

__README_DESCRIPTION = """
##Human Logs - Structured Logging Enhancement Tool

This Python script enhances the readability and manageability of logs by applying filters, 
color-coding, and additional formatting to make them easier to understand and analyze. 
It's particularly useful for developers or system administrators who deal with extensive 
log data and require a more structured and readable output.

###Features:
- Highlight Specific Fields: Bring attention to key information in the logs.
- Skip Modules: Exclude logs from specified modules unless they contain errors.
- Ignore Fields: Prevent certain fields from being displayed in the output.
- File Storage: Optionally save the processed logs to a file.
- Verbosity Control: Limit the output to log entries above a specified severity level.
- Regular Expression Filtering: Exclude log entries based on patterns in specified fields.

###Usage:
./human_logs.py [options]
"""

# Current examples from the script, appending README examples at the end
__EXAMPLES = """
###Examples

#1 ./human_logs.py -x logs.txt | less -R

#2 make start-pilorama -j5 2>&1 | ./human_logs.py

#3 Basic usage with file input:
   ./human_logs.py --input ./logs/mylog.log

#4 Using pipes for log processing:
   cat mylog.log | ./human_logs.py --verbosity DEBUG

#5 Advanced filtering:
   ./human_logs.py -hl timestamp -x -v DEBUG --require-fields body text db_statement -skm DoStep -skfp db_statement 'SELECT 1 AS ping' -ign module link span_id task_id timezone trace_id thread_id pg_conn_id stopwath_name parent_id stopwatch_units span_ref_type dp_type stopwatch_name start_timestamp libpq_wait_result_time libpq_send_query_prepared_time db_instance network_timeout_ms db_type peer_address statement_timeout_ms  -hl body text --input ./logs/mylog.log

#6  ./human_logs.py -x logs.txt | less -R

    1   \033[37mINFO  \033[32mStopping component \033[34m=> ClearComponent ( component_context_component_info.cpp:41 ) \033[0m
    2   \033[37mINFO  \033[32mStopped component \033[34m=> ClearComponent ( component_context_component_info.cpp:43 ) \033[0m
    3   \033[33mWARN  \033[32mPilorama is stopping \033[34m=> Stop ( pilorama.cpp:133 )  \033[37mcomponent_name=pilorama	coro_id=\033[38;5;14m7FCB4AE08608\033[0m


#7  make start-pilorama -j5 2>&1 | ./human_logs.py
    1   make -j4 -C build yandex-taxi-pilorama
    2   make -j4 -C build yandex-taxi-pilorama-gen-config-vars
    <...>
    155 \033[37mINFO  \033[32mStopped component \033[34m=> ClearComponent ( component_context_component_info.cpp:43 ) \033[0m

    Full logs were written into "full_logs.txt"


#8  ./human_logs.py -ign module link span_id task_id timestamp timezone trace_id stopwatch_units span_ref_type start_timestamp -x -v DEBUG logs.txt
    <...>
    94  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=statistics-storage	coro_id=\033[38;5;227m7FCB4AE08648\033[37m\033[0m
    95  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=http-client	coro_id=\033[38;5;215m7FCB4AE08680\033[37m\033[0m
    96  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=statistics-storage	coro_id=\033[38;5;227m7FCB4AE08648\033[37m\033[0m
    97  \033[32mINFO  \033[32m \033[37mcomponent_name=statistics-storage	coro_id=\033[38;5;227m7FCB4AE08648\033[37m	stopwatch_name=component_stop	total_time=0.014451\033[0m
    98  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=http-client	coro_id=\033[38;5;215m7FCB4AE08680\033[37m\033[0m
    99  \033[32mINFO  \033[32m \033[37mcomponent_name=http-client	coro_id=\033[38;5;215m7FCB4AE08680\033[37m	stopwatch_name=component_stop	total_time=2.53036\033[0m
    100 \033[32mINFO  \033[32mCall ClearComponent() for component dynamic-config \033[37m\033[0m
    <...>


#9  ./human_logs.py -ign module link span_id task_id timestamp timezone trace_id stopwatch_units span_ref_type start_timestamp -v DEBUG -hl component_name -x logs.txt
    <...>
    94  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=\033[38;5;200mstatistics-storage\033[37m	coro_id=7FCB4AE08648\033[0m
    95  \033[32mINFO  \033[32mStopping component \033[37mcomponent_name=\033[38;5;161mhttp-client\033[37m	coro_id=7FCB4AE08680\033[0m
    96  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=\033[38;5;200mstatistics-storage\033[37m	coro_id=7FCB4AE08648\033[0m
    97  \033[32mINFO  \033[32m \033[37mcomponent_name=\033[38;5;200mstatistics-storage\033[37m	coro_id=7FCB4AE08648	stopwatch_name=component_stop	total_time=0.014451\033[0m
    98  \033[32mINFO  \033[32mStopped component \033[37mcomponent_name=\033[38;5;161mhttp-client\033[37m	coro_id=7FCB4AE08680\033[0m
    99  \033[32mINFO  \033[32m \033[37mcomponent_name=\033[38;5;161mhttp-client\033[37m	coro_id=7FCB4AE08680	stopwatch_name=component_stop	total_time=2.53036\033[0m
    100 \033[32mINFO  \033[32mCall ClearComponent() for component dynamic-config \033[37m\033[0m
    <...>
   
""" # noqa: E501

def main():
    parser = argparse.ArgumentParser(
        description=__README_DESCRIPTION,
        epilog=__EXAMPLES,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        '-hl',
        '--highlights',
        nargs='+',
        default=['coro_id'],
        help='Fields to highlight in the output',
    )
    parser.add_argument(
        '-skm',
        '--skip-modules',
        nargs='+',
        default=['DoStep'],
        help='Exclude logs from the listed modules. The rule is ignored if the log level is ERROR or CRITICAL.',
    )    
    parser.add_argument(
        '-ign',
        '--ignores',
        nargs='+',
        default=['link', 'timestamp', 'timezone'],
        help='Fields to exclude from the output entirely',
    )
    parser.add_argument(
        '-f',
        '--file',
        default='full_logs.txt',
        help='Path to save the full logs',
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
        help='Print unimportant fields if log level is greater than this value (TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)',
    )
    parser.add_argument(
        '--input',
        nargs='?',
        default=sys.stdin,
        help='Input file or nothing to read from stdin',
    )
    parser.add_argument(
        '--require-fields',
        nargs='+',
        default=[],
        help='Do not display a log if none of these fields are present and non-empty. The rule is ignored if not set or if the log level is ERROR or CRITICAL.',
    )    
    parser.add_argument(
        '-skfp',
        '--skip-field-patterns',
        nargs='+',
        default=['db_statement', 'SELECT 1 AS ping'],
        help='Exclude logs where the specified field matches the given regex. Field-regex pairs must be provided. Ignored if the log level is ERROR or CRITICAL.',
    )
    args = parser.parse_args()

    if len(args.skip_field_patterns) % 2 != 0:
        raise ValueError("Warning: --skip-field-patterns requires an even number of arguments (field regex pairs)")

    h_logs = HumanLogs(
        highlights=args.highlights,
        ignores=args.ignores,
        skip_modules=args.skip_modules,
        filename=args.file,
        no_file_stores=args.no_file_stores,
        verbosity=args.verbosity,
        skip_field_patterns=args.skip_field_patterns,
        require_fields=args.require_fields,
    )

    if args.input == sys.stdin:
        h_logs.process_file(sys.stdin)
    else:
        with open(args.input) as inp:
            h_logs.process_file(inp)


if __name__ == '__main__':
    main()

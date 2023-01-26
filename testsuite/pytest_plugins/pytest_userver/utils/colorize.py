import itertools
import json
import sys

from . import tskv


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
            range(2, 7),
            range(10, 15),
            range(38, 51, 2),
            range(75, 87, 2),
            range(112, 123, 3),
            range(128, 159, 3),
            range(164, 195, 3),
            range(203, 231, 3),
        )
    ]

    @staticmethod
    def colorize(value):
        ind = hash(value) % len(Colors.__NICE_COLORS)
        return Colors.__NICE_COLORS[ind]


LEVEL_COLORS = {
    'TRACE': Colors.DARK_GRAY,
    'DEBUG': Colors.GRAY,
    'INFO': Colors.GREEN,
    'WARNING': Colors.YELLOW,
    'ERROR': Colors.RED,
    'CRITICAL': Colors.BRIGHT_RED,
    'none': Colors.DEFAULT,
}
HTTP_STATUS_COLORS = {
    '2': Colors.GREEN,
    '3': Colors.GREEN,
    '4': Colors.YELLOW,
    '5': Colors.RED,
}


class Colorizer:
    def __init__(self, *, verbose=False, colors_enabled=True):
        self._requests = {}
        self.verbose = verbose
        self.colors_enabled = colors_enabled

    def colorize_line(self, line):
        if not line.startswith('tskv\t'):
            return line
        return self.colorize_tskv(line)

    def colorize_tskv(self, line):
        row = tskv.parse_line(line)
        return self.colorize_row(row)

    def colorize_row(self, row):
        row = row.copy()
        flowid = '-'.join([row.get(key, '') for key in ('link', 'trace_id')])

        entry_type = row.pop('_type', None)
        link = row.get('link', None)
        level = row.pop('level', 'none')
        text = row.pop('text', '')

        extra_fields = []
        if entry_type == 'request':
            self._requests[link] = self._build_request_info(row)
            if 'body' in row:
                extra_fields.append(
                    'request_body='
                    + self.textcolor(
                        try_reformat_json(row.pop('body')), Colors.YELLOW,
                    ),
                )
        elif entry_type == 'response':
            if 'meta_code' in row:
                status_code = row.pop('meta_code')
                extra_fields.append(
                    self._http_status('meta_code', status_code),
                )
            if not text:
                text = 'Response finished'
            extra_fields.append(
                'response_body='
                + self.textcolor(
                    try_reformat_json(row.pop('body')), Colors.YELLOW,
                ),
            )
        elif entry_type == 'mockserver_request':
            text = 'Mockserver request finished'
            if 'meta_code' in row:
                status_code = str(row.pop('meta_code'))
                extra_fields.append(
                    self._http_status('meta_code', status_code),
                )
            for key in ('method', 'url', 'status', 'exc_info', 'delay'):
                value = row.pop(key, None)
                if value:
                    extra_fields.append(f'{key}={value}')

        if link in self._requests:
            logid = f'[{self._requests[link]}]'
        elif link is not None:
            logid = f'[{link}]'
        else:
            logid = '<userver>'

        level_color = LEVEL_COLORS.get(level)
        flow_color = Colors.colorize(flowid)

        fields = [
            self.textcolor(f'{level:<8}', level_color),
            self.textcolor(logid, flow_color),
        ]
        if text:
            fields.append(text)
        elif self.verbose:
            fields.append('<NO TEXT>')
        else:
            return None

        fields.extend(extra_fields)
        if self.verbose:
            fields.extend([f'{k}={v}' for k, v in row.items()])
        return ' '.join(fields)

    def textcolor(self, text, color):
        if not self.colors_enabled:
            return str(text)
        return f'{color}{text}{Colors.DEFAULT}'

    def _http_status(self, key, status):
        color = HTTP_STATUS_COLORS.get(status[:1], Colors.DEFAULT)
        return self.textcolor(f'{key}={status}', color)

    def _build_request_info(self, row):
        if 'uri' not in row:
            return None
        uri = row['uri']
        method = row.get('method', 'UNKNOWN')
        return f'{method} {uri}'


def format_json(obj):
    encoded = json.dumps(
        obj,
        indent=2,
        separators=(',', ': '),
        sort_keys=True,
        ensure_ascii=False,
    )
    return encoded


def try_reformat_json(body):
    try:
        # TODO: unescape string
        data = json.loads(body)
        return format_json(data)
    except ValueError:
        return body


def colorize(stream):
    colorizer = Colorizer()
    for line in stream:
        line = line.rstrip('\r\n')
        color_line = colorizer.colorize_line(line)
        if color_line is not None:
            print(color_line)


if __name__ == '__main__':
    colorize(sys.stdin)

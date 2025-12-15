from pylint.checkers import BaseRawFileChecker


def read_from_node(node):
    return [line.decode('utf-8') for line in node.stream().readlines()]


class SleepChecker(BaseRawFileChecker):
    name = 'sleep-in-test'
    priority = -1
    msgs = {
        'E9901': (
            (
                'Do not use %s in tests for synchronization! '
                'It only hides the problem and leads to flaky tests. '
                'If you have to wait for an event, either use testpoints '
                'or wait*() from pytest_userver.utils.sync module.'
            ),
            'sleep-in-test',
            'Tests should not use *.sleep() functions for synchronization.',
        ),
    }

    def process_module(self, node):
        file_content = read_from_node(node)
        for line_num, line in enumerate(file_content):
            for func in ('time.sleep', 'asyncio.sleep'):
                self.check_line(line_num, line, func)

    def check_line(self, line_num: int, line: str, func: str):
        if func not in line:
            return

        self.add_message(
            'sleep-in-test',
            line=line_num,
            args=func,
        )


def register(linter):
    linter.register_checker(SleepChecker(linter))

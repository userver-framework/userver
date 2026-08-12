import pytest


def pytest_addoption(parser: pytest.Parser) -> None:
    parser.addoption(
        '--postgresql',
        action='store',
        default=None,
        help='Injected by the local postgres recipe, ignore it.',
    )

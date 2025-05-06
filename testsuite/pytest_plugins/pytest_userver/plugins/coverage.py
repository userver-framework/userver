"""
Helpers for coverage plugins.
"""


class UncoveredError(Exception):
    pass


def collection_modifyitems(test_name, config, items) -> None:
    """
    Moves coverage tests to the end.
    """

    tests = set(config.cache.get('coverage_tests', set()))
    tests.add(test_name)
    config.cache.set('coverage_tests', list(tests))

    coverage_indices = [i for i in range(len(items)) if items[i].name in tests]

    if not coverage_indices:
        return

    modified_items = []

    for i in range(len(items)):
        if i not in coverage_indices:
            modified_items.append(items[i])

    for i in coverage_indices:
        modified_items.append(items[i])

    items[:] = modified_items

"""Utilities for sorting graphs topologically."""

from collections.abc import Iterable
from collections.abc import Mapping
from collections.abc import Sequence
from graphlib import TopologicalSorter
from typing import TypeVar

T = TypeVar('T')


def stable_toposort(*, items: Sequence[T], graph: Mapping[T, Iterable[T]]) -> list[T]:
    """
    Stable topological sort using `graphlib.TopologicalSorter`.
    items: original iterable of nodes (defines preferred order).
    edges: iterable of (u, v) edges meaning u -> v.
    Raises `graphlib.CycleError` if the graph contains a cycle.
    """
    ts = TopologicalSorter(graph)
    ts.prepare()

    item_to_index = {x: i for i, x in enumerate(items)}
    result: list[T] = []

    while ts.is_active():
        ready = list(ts.get_ready())
        ready.sort(key=lambda x: item_to_index[x])
        for node in ready:
            result.append(node)
            ts.done(node)

    return result

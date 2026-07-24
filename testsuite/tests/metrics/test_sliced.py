import pytest
import pytest_userver.metrics


def test_hello_service_greetings_stats():
    # /// [sliced snippet]
    # Suppose 'hello-handler' exposes a per-language greetings counter and a
    # couple of top-level request counters, e.g. as reported by
    # `await monitor_client.metrics(prefix='hello-handler')`:
    metrics = pytest_userver.metrics.MetricsSnapshot.from_layered_dict({
        'hello-handler.requests-total': 8,
        'hello-handler.errors-total': 0,
        'hello-handler.greetings-by-lang-count': {
            'lang = en': 3,
            'lang = ru': 5,
        },
    })

    # First `sliced()` narrows the snapshot down to the handler's own metrics,
    # dropping any unrelated components and letting us use short paths below.
    handler = metrics.sliced('hello-handler')
    assert handler.value_at('requests-total') == 8
    assert handler.value_at('errors-total') == 0

    # Second `sliced()` narrows further, down to a single metric path with
    # a varying 'lang' label; the empty string ('') stands for "no path left".
    greetings_by_lang = handler.sliced('greetings-by-lang-count')
    assert greetings_by_lang.value_at('', labels={'lang': 'en'}) == 3
    assert greetings_by_lang.value_at('', labels={'lang': 'ru'}) == 5
    # /// [sliced snippet]


def test_exact_match():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a.b': {pytest_userver.metrics.Metric({}, 5)},
    })
    sliced = values.sliced('a.b')
    assert list(sliced.keys()) == ['']
    assert sliced.value_at('') == 5


def test_strips_prefix_and_dot():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a.b.c': {pytest_userver.metrics.Metric({}, 5)},
    })
    sliced = values.sliced('a.b')
    assert list(sliced.keys()) == ['c']
    assert sliced.value_at('c') == 5


def test_partial_segment_does_not_match():
    values = pytest_userver.metrics.MetricsSnapshot({
        'calc.by-project-count': {pytest_userver.metrics.Metric({}, 5)},
    })
    # 'by-project' is not a whole '.'-separated segment of 'by-project-count'
    sliced = values.sliced('calc.by-project')
    assert list(sliced.keys()) == []


def test_unrelated_path_is_dropped():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a.b': {pytest_userver.metrics.Metric({}, 5)},
        'other.c': {pytest_userver.metrics.Metric({}, 1)},
    })
    sliced = values.sliced('a')
    assert list(sliced.keys()) == ['b']


def test_composes():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a.b.c': {pytest_userver.metrics.Metric({}, 5)},
    })
    sliced = values.sliced('a').sliced('b')
    assert list(sliced.keys()) == ['c']
    assert sliced.value_at('c') == 5

    sliced_all = values.sliced('a').sliced('b').sliced('c')
    assert list(sliced_all.keys()) == ['']
    assert sliced_all.value_at('') == 5


def test_with_labels_filters_by_subset():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a': {
            pytest_userver.metrics.Metric({'rsp': 'x'}, 1),
            pytest_userver.metrics.Metric({'rsp': 'y'}, 2),
        },
    })
    sliced = values.sliced('a', {'rsp': 'x'})
    assert sliced.value_at('') == 1


def test_with_labels_composes():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a': {
            pytest_userver.metrics.Metric({'rsp': 'x', 'extra': 'e'}, 1),
            pytest_userver.metrics.Metric({'rsp': 'y', 'extra': 'e'}, 2),
        },
    })
    sliced = values.sliced('a', {'rsp': 'x'}).sliced('', {'extra': 'e'})
    assert sliced.value_at('') == 1


def test_with_labels_allows_passing_only_remaining_labels_to_value_at():
    values = pytest_userver.metrics.MetricsSnapshot({
        'handle-rps-data.starts': {
            pytest_userver.metrics.Metric({'handle_name': 'h1', 'source_name': 'unknown'}, 5),
            pytest_userver.metrics.Metric({'handle_name': 'h2', 'source_name': 'unknown'}, 3),
        },
    })
    sliced = values.sliced('handle-rps-data.starts', {'handle_name': 'h1'})
    # Only the still-varying label needs to be passed; 'handle_name' was
    # already fixed by `sliced()` and is implicitly merged in.
    assert sliced.value_at('', {'source_name': 'unknown'}) == 5


def test_with_labels_allows_passing_only_remaining_labels_to_metrics_at():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a': {
            pytest_userver.metrics.Metric({'rsp': 'x', 'extra': 'e'}, 1),
            pytest_userver.metrics.Metric({'rsp': 'y', 'extra': 'e'}, 2),
        },
    })
    sliced = values.sliced('a', {'rsp': 'x'})
    (metric,) = sliced.metrics_at('', require_labels={'extra': 'e'})
    assert metric.value == 1


def test_with_labels_still_allows_passing_the_full_label_set():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a': {
            pytest_userver.metrics.Metric({'rsp': 'x', 'extra': 'e'}, 1),
        },
    })
    sliced = values.sliced('a', {'rsp': 'x'})
    # Passing the already-sliced label again, with the same value, still works.
    assert sliced.value_at('', {'rsp': 'x', 'extra': 'e'}) == 1


def test_does_not_touch_metric_objects():
    metric = pytest_userver.metrics.Metric({'a': 'b'}, 5)
    values = pytest_userver.metrics.MetricsSnapshot({'a.b': {metric}})
    sliced = values.sliced('a')
    (sliced_metric,) = sliced['b']
    assert sliced_metric is metric


def test_prefix_ending_with_dot_fails():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b': {pytest_userver.metrics.Metric({}, 1)}})
    with pytest.raises(AssertionError):
        values.sliced('a.')


def test_ambiguous_trailing_dot_fails():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b.': {pytest_userver.metrics.Metric({}, 1)}})
    with pytest.raises(AssertionError):
        values.sliced('a.b')


def test_ambiguous_double_dot_fails():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b..c': {pytest_userver.metrics.Metric({}, 1)}})
    with pytest.raises(AssertionError):
        values.sliced('a.b')


def test_unsliced_on_never_sliced_snapshot_is_equivalent():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b': {pytest_userver.metrics.Metric({}, 5)}})
    unsliced = values.unsliced()
    assert unsliced == values
    assert unsliced is not values


def test_unsliced_restores_full_path():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b': {pytest_userver.metrics.Metric({}, 5)}})
    sliced = values.sliced('a')
    unsliced = sliced.unsliced()
    assert list(unsliced.keys()) == ['a.b']
    assert unsliced.value_at('a.b') == 5


def test_unsliced_restores_full_path_after_exact_match_slice():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b': {pytest_userver.metrics.Metric({}, 5)}})
    sliced = values.sliced('a.b')
    unsliced = sliced.unsliced()
    assert list(unsliced.keys()) == ['a.b']
    assert unsliced.value_at('a.b') == 5


def test_unsliced_restores_full_path_after_chained_slice():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b.c': {pytest_userver.metrics.Metric({}, 5)}})
    sliced = values.sliced('a').sliced('b')
    unsliced = sliced.unsliced()
    assert list(unsliced.keys()) == ['a.b.c']
    assert unsliced.value_at('a.b.c') == 5


def test_unsliced_does_not_restore_filtered_out_metrics():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a.b': {pytest_userver.metrics.Metric({}, 5)},
        'other.c': {pytest_userver.metrics.Metric({}, 1)},
    })
    sliced = values.sliced('a')
    unsliced = sliced.unsliced()
    assert 'other.c' not in unsliced
    assert list(unsliced.keys()) == ['a.b']


def test_unsliced_returns_new_object():
    values = pytest_userver.metrics.MetricsSnapshot({'a.b': {pytest_userver.metrics.Metric({}, 5)}})
    sliced = values.sliced('a')
    unsliced = sliced.unsliced()
    assert unsliced is not sliced
    assert unsliced is not values


def test_unsliced_resets_sliced_labels():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a': {
            pytest_userver.metrics.Metric({'rsp': 'x', 'extra': 'e'}, 1),
            pytest_userver.metrics.Metric({'rsp': 'y', 'extra': 'e'}, 2),
        },
    })
    sliced = values.sliced('a', {'rsp': 'x'})
    # Before unsliced(): the 'rsp' label sliced by is implicitly merged in.
    assert sliced.value_at('', {'extra': 'e'}) == 1

    unsliced = sliced.unsliced()
    # After unsliced(): full path is restored, and sliced labels no longer
    # apply implicitly, so the full label set must be passed again.
    assert unsliced.value_at('a', {'rsp': 'x', 'extra': 'e'}) == 1
    with pytest.raises(AssertionError):
        unsliced.value_at('a', {'extra': 'e'})


def test_value_at_missing_path_error_mentions_sliced_prefix():
    # Regression test: when `sliced=True` is used together with an exact
    # `path=` and that path does not actually exist, `value_at('')` used to
    # raise `No metrics found by path ""`, which does not mention what was
    # actually being looked up. The error message must include the prefix
    # that was sliced off, so the missing path can be diagnosed without
    # having to rerun the query without `sliced=True`.
    values = pytest_userver.metrics.MetricsSnapshot({
        'a.b': {pytest_userver.metrics.Metric({}, 5)},
    })
    sliced = values.sliced('wrong-prefix')
    with pytest.raises(AssertionError, match='No metrics found by path "" after slicing "wrong-prefix"'):
        sliced.value_at('')


def test_value_at_missing_path_error_omits_prefix_when_not_sliced():
    values = pytest_userver.metrics.MetricsSnapshot({
        'a.b': {pytest_userver.metrics.Metric({}, 5)},
    })
    with pytest.raises(AssertionError, match=r'No metrics found by path "c"$'):
        values.value_at('c')

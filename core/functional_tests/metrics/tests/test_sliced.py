# /// [sliced functional test]
async def test_sliced(monitor_client):
    # Without sliced=True: every call repeats the common prefix and the common label.
    metrics = await monitor_client.metrics(prefix='cache.any', labels={'cache_name': 'sample-cache'})
    assert metrics.value_at('cache.any.update.attempts_count', {'cache_name': 'sample-cache'}) >= 0
    assert metrics.value_at('cache.any.update.failures_count', {'cache_name': 'sample-cache'}) == 0

    # With sliced=True: the prefix and the label are stripped once, then reused implicitly.
    metrics = await monitor_client.metrics(prefix='cache.any', labels={'cache_name': 'sample-cache'}, sliced=True)
    assert metrics.value_at('update.attempts_count') >= 0
    assert metrics.value_at('update.failures_count') == 0
    # /// [sliced functional test]

class RegisteredTrx:
    """
    RegisteredTrx maintains transaction fault injection state to test
    transaction failure code path.

    You may enable specific transaction failure calling `enable_trx_failure`
    on that transaction name. After that, the transaction's `Commit` method
    will throw an exception.

    If you don't need a fault injection anymore (e.g. you want to test
    a successfull retry), you may call `disable_trx_failure` afterwards.

    Example usage:
    @snippet integration_tests/tests/test_trx_failure.py  fault injection

    @ingroup userver_testsuite
    """

    def __init__(self):
        self._registered_trx = set()

    def enable_failure(self, name: str) -> None:
        self._registered_trx.add(name)

    def disable_failure(self, name: str) -> None:
        if self.is_failure_enabled(name):
            self._registered_trx.remove(name)

    def is_failure_enabled(self, name: str) -> bool:
        return name in self._registered_trx

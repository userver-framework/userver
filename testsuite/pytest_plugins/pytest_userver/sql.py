class RegisteredTrx:
    """
    RegisteredTrx maintains transaction fault injection state to test
    transaction failure code path.

    You may enable specific transaction failure calling `enable_trx_failure`
    on that transaction name. After that, the transaction's `Commit` method
    will throw an exception.

    If you don't need a fault injection anymore (e.g. you want to test
    a successfull retry), you may call `disable_trx_failure` afterwards.

    @ingroup userver_testsuite
    """

    def __init__(self):
        self._registered_trx = set()

    def enable_failure(self, name):
        self._registered_trx.add(name)

    def disable_failure(self, name):
        if self.is_failure_enabled(name):
            self._registered_trx.remove(name)

    def is_failure_enabled(self, name):
        return name in self._registered_trx

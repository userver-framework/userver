import asyncio
from collections.abc import Callable
import datetime
import inspect
import logging
from typing import Any

TOTAL_WAIT_SECONDS = 30
ITERATION_PERIOD_SECONDS = 0.5

logger = logging.getLogger(__name__)


class NotReady(Exception):
    pass


async def wait_until(
    func: Callable,
    *,
    relax_period_seconds: float = ITERATION_PERIOD_SECONDS,
    total_wait_seconds: float = TOTAL_WAIT_SECONDS,
) -> Any:
    """
    Waits for some external event for `total_wait_seconds` and
    calls `func` every `relax_period_seconds`.
    If `func` raises NotReady exception, the waiting continues.
    If `func` succesfully returns, wait_until() returns the same value.
    After `total_wait_seconds` of unsuccesfull checks wait_until()
    raises TimeoutError.

    Example:

    .. code-block:: python

        async def try_to_connect_db():
            if not db_conn_is_ok():
                raise NotReady()

        await wait_until(try_to_connect_db)

    @throws TimeoutError after `total_wait_seconds` of unsuccesfull checks
    @note If possible, use @ref testpoint instead. @ref testpoint is
    an explicit message from the server "I'm ready", while wait_until()
    uses an implicit idea "A condition is met, so I can continue".

    @ingroup userver_testsuite
    """
    start = datetime.datetime.now()
    while datetime.datetime.now() - start < datetime.timedelta(seconds=total_wait_seconds):
        try:
            return await func()
        except NotReady:
            await asyncio.sleep(relax_period_seconds)

    raise TimeoutError()


async def wait(
    check: Callable,
    *,
    catch: Any | tuple = (),
    relax_period_seconds: float = ITERATION_PERIOD_SECONDS,
    total_wait_seconds: float = TOTAL_WAIT_SECONDS,
    failure_msg: str = 'Timeout happened while waiting for an event',
    relax_msg: str | None = None,
) -> None:
    """
    Waits for some external event for `total_wait_seconds` and
    calls `check` every `relax_period_seconds`.
    If `check` returns False or raises NotReady exception
    (or any exception type referenced in `catch`), the waiting continues.
    If `check` returns True, wait_until() stops and returns.
    After `total_wait_seconds` of unsuccesfull checks wait_until()
    raises TimeoutError.

    Example:

    .. code-block:: python

        async def is_ready():
            return await conn.is_ready()

        await wait_until(try_to_connect_db)

    @throws TimeoutError after `total_wait_seconds` of unsuccesfull checks
    @note If possible, use @ref testpoint instead. @ref testpoint is
    an explicit message from the server "I'm ready", while wait_until()
    uses an implicit idea "A condition is met, so I can continue".

    @ingroup userver_testsuite
    """

    if isinstance(catch, tuple):
        exceptions = catch
    else:
        exceptions = (catch,)
    exceptions = (NotReady, *exceptions)

    async def func():
        try:
            result = check()
            if inspect.isawaitable(result):
                ok = await result
            else:
                ok = result
            if ok:
                return
        except exceptions:
            pass

        logger.warning('%s', relax_msg)
        raise NotReady from None

    try:
        await wait_until(
            func,
            relax_period_seconds=relax_period_seconds,
            total_wait_seconds=total_wait_seconds,
        )
    except TimeoutError:
        assert False, failure_msg

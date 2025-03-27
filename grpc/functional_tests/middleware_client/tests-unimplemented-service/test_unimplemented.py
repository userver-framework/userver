import re

import pytest
import pytest_userver.client
import pytest_userver.grpc

SAY_HELLO = "'samples.api.GreeterService/SayHello'"


# Overriding testsuite fixture
@pytest.fixture(name='asyncexc_check')
def _asyncexc_check(asyncexc_check):
    """
    We've inserted testsuite's asyncexc_check after calls to the service client, but for the current tests these checks
    are an impediment, as they would hide the errors thrown from ugrpc client.

    Disable auto-check of background errors by default for tests in this file..
    """

    def check(*, really_check: bool = False):
        if really_check:
            asyncexc_check()

    return check


async def test_service_missing(service_client, asyncexc_check):
    message = 'Method not found!'
    expected_error = re.compile(f"{SAY_HELLO} failed: code=UNIMPLEMENTED, message='{message}'")

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed, match=expected_error):
        await service_client.run_task('call-say-hello')

    # asyncexc check currently does not complain in this case :(
    # This is a golden test for the current behavior, which will hopefully be fixed in the future.
    asyncexc_check(really_check=True)

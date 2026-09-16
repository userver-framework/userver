IF (OS_SDK == "ubuntu-22" OR OS_SDK == "ubuntu-24" OR OS_SDK == "ubuntu-26")
    SUBSCRIBER(g:taxi-common)

    PY3TEST()

    SIZE(SMALL)

    ALL_PYTEST_SRCS()

    PEERDIR(
        taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver
        taxi/uservices/userver-arc-utils/functional_tests/pytest_plugins
    )

    DEPENDS(
        taxi/uservices/userver/core/functional_tests/jemalloc
    )

    DATA(
        arcadia/taxi/uservices/userver/core/functional_tests/jemalloc
    )

    CONFTEST_LOAD_POLICY_LOCAL()
    TEST_CWD(/)

    END()
ENDIF()

PACKAGE()

IF(OS_LINUX)
FROM_SANDBOX(
    4550745373
    RENAME redis-7.2.linux.x86_64/redis-server
    OUT_NOAUTO bin/redis-server EXECUTABLE
)
FROM_SANDBOX(
    4550745373
    RENAME redis-7.2.linux.x86_64/redis-cli
    OUT_NOAUTO bin/redis-cli EXECUTABLE
)
ELSEIF(OS_DARWIN)
FROM_SANDBOX(
    4544064398
    RENAME redis-stack-server-7.2.0-M01.catalina.x86_64/bin/redis-cli
    OUT_NOAUTO bin/redis-cli EXECUTABLE
)
FROM_SANDBOX(
    4544064398
    RENAME redis-stack-server-7.2.0-M01.catalina.x86_64/bin/redis-server
    OUT_NOAUTO bin/redis-server EXECUTABLE
)
ENDIF()

END()

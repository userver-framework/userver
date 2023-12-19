G_BENCHMARK()

OWNER(g:taxi-common)

SIZE(MEDIUM)


INCLUDE(
    ${ARCADIA_ROOT}/taxi/external/recipes/redis/recipe.inc
)

PEERDIR(
    contrib/libs/hiredis
    contrib/libs/libev
    taxi/uservices/userver/redis
)

ADDINCL(
    taxi/uservices/userver/core/src
    taxi/uservices/userver/redis/src
    taxi/uservices/userver/universal/src
)

SRCS(
    redis_fixture.cpp
    redis_benchmark.cpp
)

END()

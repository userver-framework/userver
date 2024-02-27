OWNER(vaninvv)

PY3_LIBRARY()

PY_SRCS(
    pytest_plugin.py
)

PEERDIR(
    contrib/python/pytest
    contrib/python/redis
)

RESOURCE(configs/cluster.conf redis_cluster_config)

END()

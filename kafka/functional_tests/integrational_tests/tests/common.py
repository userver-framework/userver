import os


CONSUME_BASE_ROUTE = '/consume'
PRODUCE_ROUTE = '/produce'


def get_kafka_conf() -> dict[str, str]:
    return {'bootstrap.servers': os.getenv('KAFKA_RECIPE_BROKER_LIST')}

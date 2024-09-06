import ydb as ydb_native_client


class YdbClient:
    def __init__(self, endpoint, database):
        self._driver = self._init_driver(endpoint, database)
        self._database = database
        self._session = self._driver.table_client.session().create()

    def execute(self, query):
        return self._session.transaction().execute(query, commit_tx=True)

    @property
    def session(self):
        return self._session

    @property
    def database(self):
        return self._database

    @staticmethod
    def _init_driver(endpoint, database):
        config = ydb_native_client.DriverConfig(
            endpoint=endpoint, database=database, auth_token='',
        )
        driver = ydb_native_client.Driver(config)
        driver.wait(timeout=30)
        return driver


def _prepare_column(column, version=None):
    column_type = None
    if version is None or version == 1:
        column_type = ydb_native_client.OptionalType(
            getattr(ydb_native_client.PrimitiveType, column['type']),
        )
    elif column['type'][-1] == '?':
        column_type = ydb_native_client.OptionalType(
            getattr(ydb_native_client.PrimitiveType, column['type'][:-1]),
        )
    else:
        column_type = getattr(ydb_native_client.PrimitiveType, column['type'])

    return ydb_native_client.Column(column['name'], column_type)


def _prepare_index(index):
    return ydb_native_client.TableIndex(index['name']).with_index_columns(
        *tuple(index['index_columns']),
    )


def create_table(client, schema):
    version = schema.get('syntax_version', None)
    client.session.create_table(
        '/{}/{}'.format(client.database, schema['path']),
        ydb_native_client.TableDescription()
        .with_primary_keys(*schema['primary_key'])
        .with_columns(
            *[_prepare_column(column, version) for column in schema['schema']],
        )
        .with_indexes(
            *[_prepare_index(index) for index in schema.get('indexes', [])],
        ),
    )


def drop_table(client, table):
    try:
        client.session.drop_table('/{}/{}'.format(client.database, table))
    except:  # noqa pylint: disable=bare-except
        pass

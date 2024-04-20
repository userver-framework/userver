#pragma once

#include <libpq-fe.h>

#ifdef __cplusplus
extern "C" {
#endif

/// This is a workaround function for libpq is not handling
/// the `portal suspended` backend response message
extern PGresult* PQXgetResult(PGconn* conn, const PGresult* description);

/// This is a workaround function for libpq is not handling
/// the `portal suspended` backend response message
extern int PQXisBusy(PGconn* conn, const PGresult* description);

/// This is a workaround function for libpq always sending D(secribe) message
/// for prepared statements. The description doesn't change, hence no need to do
/// that unconditionally. See
/// https://www.postgresql.org/message-id/82c438b4-d91c-9009-65fc-593124d5a277%40yandex.ru
extern int PQXsendQueryPrepared(PGconn* conn, const char* stmtName, int nParams,
                                const char* const* paramValues,
                                const int* paramLengths,
                                const int* paramFormats, int resultFormat,
                                PGresult* description);

extern int PQXpipelinePutSync(PGconn* conn);

#ifdef __cplusplus
}
#endif

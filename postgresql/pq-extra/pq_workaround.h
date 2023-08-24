#pragma once

#include <libpq-fe.h>

#ifdef __cplusplus
extern "C" {
#endif

/// This is a workaround function for libpq is not handling
/// the `portal suspended` backend response message
extern PGresult* PQXgetResult(PGconn* conn);

/// This is a workaround function for libpq is not handling
/// the `portal suspended` backend response message
extern int PQXisBusy(PGconn* conn);

#ifdef __cplusplus
}
#endif

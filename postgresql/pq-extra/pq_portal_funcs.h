#pragma once

#include <libpq-fe.h>

#ifdef __cplusplus
extern "C" {
#endif

/***
 * Send a bind portal message to PostgreSQL server
 * @param conn          Connection handle
 * @param stmt_name     Name of prepared statement, may be empty string. The
 *                      query must have been already prepared. NULL is not
 *                      allowed here.
 * @param portal_name   Name of portal being bound, may be empty string. NULL
 *                      is not allowed here.
 * @param n_params      Number of query parameters.
 * @param param_values  Parameter values, see PQSendQueryParams for description
 * @param param_lengths Parameter lengths, see PQSendQueryParams for description
 * @param param_formats Parameter formats, see PQSendQueryParams for description
 * @param result_format Result format, 0 for text, 1 for binary
 * @return              1 if the message was sent, 0 in case of error, please
 *                      check PQerrorMessage for error message
 */
extern int PQXSendPortalBind(PGconn* conn, const char* stmt_name,
                             const char* portal_name, int n_params,
                             const char* const* param_values,
                             const int* param_lengths, const int* param_formats,
                             int result_format);

/**
 * Send an execute portal message to PostgreSQL server
 * @param conn        Connection handle
 * @param portal_name Name of portal, must not be NULL. The portal must have
 *                    been bound already.
 * @param n_rows      Number of rows to fetch.
 * @return            1 if the message was sent, 0 in case of error, please
 *                    check PQerrorMessage for error message
 */
extern int PQXSendPortalExecute(PGconn* conn, const char* portal_name,
                                int n_rows);

#ifdef __cplusplus
}
#endif

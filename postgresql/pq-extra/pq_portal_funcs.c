#include "pq_portal_funcs.h"

#include <internal/postgres_fe.h>

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include <internal/libpq-int.h>

/*
 * Common startup code for PQsendQuery and sibling routines
 *
 * Copy-paste (almost) of static function code from fe-exec.c
 * Check for protocol version is added to this function
 */
static bool PQXsendQueryStart(PGconn* conn) {
  if (!conn) return false;

  /* clear the error string */
  resetPQExpBuffer(&conn->errorMessage);

  /* This isn't gonna work on a 2.0 server */
  if (PG_PROTOCOL_MAJOR(conn->pversion) < 3) {
    printfPQExpBuffer(
        &conn->errorMessage,
        libpq_gettext("function requires at least protocol version 3.0\n"));
    return false;
  }

  /* Don't try to send if we know there's no live connection. */
  if (conn->status != CONNECTION_OK) {
    printfPQExpBuffer(&conn->errorMessage,
                      libpq_gettext("no connection to the server\n"));
    return false;
  }
  /* Can't send while already busy, either. */
  if (conn->asyncStatus != PGASYNC_IDLE) {
    printfPQExpBuffer(
        &conn->errorMessage,
        libpq_gettext("another command is already in progress\n"));
    return false;
  }

  /* initialize async result-accumulation state */
  pqClearAsyncResult(conn);

  /* reset single-row processing mode */
  conn->singleRowMode = false;

  /* ready to send command message */
  return true;
}

/*
 * PQXSendPortalBind
 * The query must have been already prepared
 */
int PQXSendPortalBind(PGconn* conn, const char* stmt_name,
                      const char* portal_name, int n_params,
                      const char* const* param_values, const int* param_lengths,
                      const int* param_formats, int result_format) {
  if (!PQXsendQueryStart(conn)) return 0;

  /* check the arguments */
  if (!stmt_name) {
    printfPQExpBuffer(&conn->errorMessage,
                      "statement name is a null pointer\n");
    return 0;
  }
  if (!portal_name) {
    printfPQExpBuffer(&conn->errorMessage, "portal name is a null pointer\n");
    return 0;
  }
  if (n_params < 0 || n_params > 65535) {
    printfPQExpBuffer(&conn->errorMessage,
                      "number of parameters must be between 0 and 65535\n");
    return 0;
  }

  if (conn->xactStatus != PQTRANS_INTRANS) {
    printfPQExpBuffer(&conn->errorMessage,
                      "a transaction is needed for a portal to work\n");
    return 0;
  }

  /* Construct the Bind message */
  if (pqPutMsgStart('B', false, conn) < 0 || pqPuts(portal_name, conn) < 0 ||
      pqPuts(stmt_name, conn) < 0)
    return 0;

  /* Send parameter formats */
  if (n_params > 0 && param_formats) {
    if (pqPutInt(n_params, 2, conn) < 0) return 0;
    for (int i = 0; i < n_params; ++i) {
      if (pqPutInt(param_formats[i], 2, conn) < 0) return 0;
    }
  } else {
    if (pqPutInt(0, 2, conn) < 0) return 0;
  }

  if (pqPutInt(n_params, 2, conn) < 0) return 0;

  /* Send parameters */
  for (int i = 0; i < n_params; ++i) {
    if (param_values && param_values[i]) {
      int nbytes = 0;
      if (param_formats && param_formats[i] != 0) {
        /* binary parameter */
        if (param_lengths) {
          nbytes = param_lengths[i];
        } else {
          // Format message buffer
          printfPQExpBuffer(&conn->errorMessage,
                            "length must be given for binary parameter\n");
          return 0;
        }
      } else {
        /* text parameter, do not use param_lengths */
        nbytes = strnlen(param_values[i], INT_MAX);
      }
      if (pqPutInt(nbytes, 4, conn) < 0 ||
          pqPutnchar(param_values[i], nbytes, conn) < 0)
        return 0;
    } else {
      /* take the param as NULL */
      if (pqPutInt(-1, 4, conn) < 0) return 0;
    }
  }
  /* Send result format */
  if (pqPutInt(1, 2, conn) < 0 || pqPutInt(result_format, 2, conn) < 0)
    return 0;
  if (pqPutMsgEnd(conn) < 0) return 0;

  /* construct the Sync message, not sure it is required */
  if (pqPutMsgStart('S', false, conn) < 0 || pqPutMsgEnd(conn) < 0)
    return 0;

  /* remember we are using extended query protocol */
  conn->queryclass = PGQUERY_EXTENDED;
  /* we don't have a statement, so we just need to clear it */
  if (conn->last_query) free(conn->last_query);
  conn->last_query = NULL;

  /*
   * Give the data a push.  In nonblock mode, don't complain if we're unable
   * to send it all; PQgetResult() will do any additional flushing needed.
   */
  if (pqFlush(conn) < 0) return 0;

  /* OK, it's launched! */
  conn->asyncStatus = PGASYNC_BUSY;
  return 1;
}

/*
 * PQXSendPortalExecute
 * The portal must have been already bound
 */
int PQXSendPortalExecute(PGconn* conn, const char* portal_name, int n_rows) {
  if (!PQXsendQueryStart(conn)) return 0;

  /* check the arguments */
  if (!portal_name) {
    printfPQExpBuffer(&conn->errorMessage, "portal name is a null pointer\n");
    return 0;
  }
  if (n_rows < 0) {
    printfPQExpBuffer(&conn->errorMessage,
                      "number of rows must be a non-negative number\n");
    return 0;
  }

  /* construct the Describe Portal message */
  if (pqPutMsgStart('D', false, conn) < 0 || pqPutc('P', conn) < 0 ||
      pqPuts(portal_name, conn) < 0 || pqPutMsgEnd(conn) < 0)
    return 0;

  /* construct the Execute message */
  if (pqPutMsgStart('E', false, conn) < 0 || pqPuts(portal_name, conn) < 0 ||
      pqPutInt(n_rows, 4, conn) < 0 || pqPutMsgEnd(conn) < 0)
    return 0;

  /* construct the Sync message */
  if (pqPutMsgStart('S', false, conn) < 0 || pqPutMsgEnd(conn) < 0)
    return 0;

  /* remember we are using extended query protocol */
  conn->queryclass = PGQUERY_EXTENDED;
  /* we don't have a statement, so we just need to clear it */
  if (conn->last_query) free(conn->last_query);
  conn->last_query = NULL;

  /*
   * Give the data a push.  In nonblock mode, don't complain if we're unable
   * to send it all; PQgetResult() will do any additional flushing needed.
   */
  if (pqFlush(conn) < 0) return 0;

  /* OK, it's launched! */
  conn->asyncStatus = PGASYNC_BUSY;
  return 1;
}

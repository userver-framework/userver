/*
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 * Portions Copyright (c) 1994, The Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted, provided that the above copyright notice and this
 * paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 */

#include "pq_portal_funcs.h"
#include "pq_extra_defs.h"

#include <postgres_fe.h>

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include <libpq-int.h>

/* Glue to simplify working with differing interfaces between versions */
#if PG_VERSION_NUM >= 140000
#define updatePQXExpBufferStr appendPQExpBufferStr
#define pqxPutMsgStart3 pqPutMsgStart

#include "pq_pipeline_funcs.i"
#else
#define updatePQXExpBufferStr printfPQExpBuffer

static int pqxPutMsgStart3(char msg_type, PGconn* conn) {
  return pqPutMsgStart(msg_type, false, conn);
}
#endif

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
    updatePQXExpBufferStr(
        &conn->errorMessage,
        libpq_gettext("function requires at least protocol version 3.0\n"));
    return false;
  }

  /* Don't try to send if we know there's no live connection. */
  if (conn->status != CONNECTION_OK) {
    updatePQXExpBufferStr(&conn->errorMessage,
                          libpq_gettext("no connection to the server\n"));
    return false;
  }

  /* Can't send while already busy, either, unless enqueuing for later */
  if (conn->asyncStatus != PGASYNC_IDLE
#if PG_VERSION_NUM >= 140000
      && conn->pipelineStatus == PQ_PIPELINE_OFF
#endif
  ) {
    updatePQXExpBufferStr(
        &conn->errorMessage,
        libpq_gettext("another command is already in progress\n"));
    return false;
  }

#if PG_VERSION_NUM >= 140000
  if (conn->pipelineStatus != PQ_PIPELINE_OFF) {
    /*
     * When enqueuing commands we don't change much of the connection
     * state since it's already in use for the current command. The
     * connection state will get updated when pqPipelineProcessQueue()
     * advances to start processing the queued message.
     *
     * Just make sure we can safely enqueue given the current connection
     * state. We can enqueue behind another queue item, or behind a
     * non-queue command (one that sends its own sync), but we can't
     * enqueue if the connection is in a copy state.
     */
    switch (conn->asyncStatus) {
      case PGASYNC_IDLE:
#if PG_VERSION_NUM >= 140005
      case PGASYNC_PIPELINE_IDLE:
#endif
      case PGASYNC_READY:
      case PGASYNC_READY_MORE:
      case PGASYNC_BUSY:
        /* ok to queue */
        break;
      case PGASYNC_COPY_IN:
      case PGASYNC_COPY_OUT:
      case PGASYNC_COPY_BOTH:
        appendPQExpBufferStr(
            &conn->errorMessage,
            libpq_gettext("cannot queue commands during COPY\n"));
        return false;
    }
  } else {
#else
  {
#endif
    /*
     * This command's results will come in immediately. Initialize async
     * result-accumulation state
     */
    pqClearAsyncResult(conn);

    /* reset single-row processing mode */
    conn->singleRowMode = false;
  }
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
    updatePQXExpBufferStr(&conn->errorMessage,
                          "statement name is a null pointer\n");
    return 0;
  }
  if (!portal_name) {
    updatePQXExpBufferStr(&conn->errorMessage,
                          "portal name is a null pointer\n");
    return 0;
  }
  if (n_params < 0 || n_params > 65535) {
    updatePQXExpBufferStr(&conn->errorMessage,
                          "number of parameters must be between 0 and 65535\n");
    return 0;
  }

  /*
   * A transaction must have been already started.
   * In case of pipeline mode a begin statement should already be in a pending
   * command queue. We do not actually check because pipelined queries are only
   * used in transaction blocks. If that is not the case, the portal will not be
   * created and the exception will be thrown on an attempt to fetch data.
   */
  if (conn->xactStatus != PQTRANS_INTRANS
#if PG_VERSION_NUM >= 140000
      && (conn->pipelineStatus == PQ_PIPELINE_OFF ||
          conn->asyncStatus == PGASYNC_IDLE)
#endif
  ) {
    updatePQXExpBufferStr(&conn->errorMessage,
                          "a transaction is needed for a portal to work\n");
    return 0;
  }

#if PG_VERSION_NUM >= 140000
  PGcmdQueueEntry* entry;

  entry = pqAllocCmdQueueEntry(conn);
  if (entry == NULL) return 0; /* error msg already set */
#endif

  /* Construct the Bind message */
  if (pqxPutMsgStart3('B', conn) < 0 || pqPuts(portal_name, conn) < 0 ||
      pqPuts(stmt_name, conn) < 0)
    goto sendFailed;

  /* Send parameter formats */
  if (n_params > 0 && param_formats) {
    if (pqPutInt(n_params, 2, conn) < 0) goto sendFailed;
    for (int i = 0; i < n_params; ++i) {
      if (pqPutInt(param_formats[i], 2, conn) < 0) goto sendFailed;
    }
  } else {
    if (pqPutInt(0, 2, conn) < 0) goto sendFailed;
  }

  if (pqPutInt(n_params, 2, conn) < 0) goto sendFailed;

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
          updatePQXExpBufferStr(
              &conn->errorMessage,
              libpq_gettext("length must be given for binary parameter\n"));
          goto sendFailed;
        }
      } else {
        /* text parameter, do not use param_lengths */
        nbytes = strnlen(param_values[i], INT_MAX);
      }
      if (pqPutInt(nbytes, 4, conn) < 0 ||
          pqPutnchar(param_values[i], nbytes, conn) < 0)
        goto sendFailed;
    } else {
      /* take the param as NULL */
      if (pqPutInt(-1, 4, conn) < 0) goto sendFailed;
    }
  }
  /* Send result format */
  if (pqPutInt(1, 2, conn) < 0 || pqPutInt(result_format, 2, conn))
    goto sendFailed;
  if (pqPutMsgEnd(conn) < 0) goto sendFailed;

    /* construct the Sync message if not in pipeline mode */
#if PG_VERSION_NUM >= 140000
  if (conn->pipelineStatus == PQ_PIPELINE_OFF)
#endif
  {
    if (pqxPutMsgStart3('S', conn) < 0 || pqPutMsgEnd(conn) < 0)
      goto sendFailed;
  }

#if PG_VERSION_NUM >= 140000
  /* this query has non-standard flow, using custom class */
  entry->queryclass = PGXQUERY_BIND;
#else
  /* this query has non-standard flow, using custom class */
  conn->queryclass = PGXQUERY_BIND;
  /* we don't have a statement, so we just need to clear it */
  if (conn->last_query) free(conn->last_query);
  conn->last_query = NULL;
#endif

  /*
   * Give the data a push (in pipeline mode, only if we're past the size
   * threshold).  In nonblock mode, don't complain if we're unable to send
   * it all; PQgetResult() will do any additional flushing needed.
   */
#if PG_VERSION_NUM >= 140000
  if (pqPipelineFlush(conn) < 0)
#else
  if (pqFlush(conn) < 0)
#endif
    goto sendFailed;

    /* OK, it's launched! */
#if PG_VERSION_NUM >= 140000
  pqAppendCmdQueueEntry(conn, entry);
#else
  conn->asyncStatus = PGASYNC_BUSY;
#endif
  return 1;

sendFailed:
#if PG_VERSION_NUM >= 140000
  pqRecycleCmdQueueEntry(conn, entry);
#endif
  /* error message should be set up already */
  return 0;
}

/*
 * PQXSendPortalExecute
 * The portal must have been already bound
 */
int PQXSendPortalExecute(PGconn* conn, const char* portal_name, int n_rows) {
  if (!PQXsendQueryStart(conn)) return 0;

  /* check the arguments */
  if (!portal_name) {
    updatePQXExpBufferStr(&conn->errorMessage,
                          "portal name is a null pointer\n");
    return 0;
  }
  if (n_rows < 0) {
    updatePQXExpBufferStr(&conn->errorMessage,
                          "number of rows must be a non-negative number\n");
    return 0;
  }

#if PG_VERSION_NUM >= 140000
  PGcmdQueueEntry* entry;

  entry = pqAllocCmdQueueEntry(conn);
  if (entry == NULL) return 0; /* error msg already set */
#endif

  /* construct the Describe Portal message */
  if (pqxPutMsgStart3('D', conn) < 0 || pqPutc('P', conn) < 0 ||
      pqPuts(portal_name, conn) < 0 || pqPutMsgEnd(conn) < 0)
    goto sendFailed;

  /* construct the Execute message */
  if (pqxPutMsgStart3('E', conn) < 0 || pqPuts(portal_name, conn) < 0 ||
      pqPutInt(n_rows, 4, conn) < 0 || pqPutMsgEnd(conn) < 0)
    goto sendFailed;

    /* construct the Sync message if not in pipeline mode */
#if PG_VERSION_NUM >= 140000
  if (conn->pipelineStatus == PQ_PIPELINE_OFF)
#endif
  {
    if (pqxPutMsgStart3('S', conn) < 0 || pqPutMsgEnd(conn) < 0)
      goto sendFailed;
  }

#if PG_VERSION_NUM >= 140000
  /* remember we are using extended query protocol */
  entry->queryclass = PGQUERY_EXTENDED;
#else
  /* remember we are using extended query protocol */
  conn->queryclass = PGQUERY_EXTENDED;
  /* we don't have a statement, so we just need to clear it */
  if (conn->last_query) free(conn->last_query);
  conn->last_query = NULL;
#endif

  /*
   * Give the data a push (in pipeline mode, only if we're past the size
   * threshold).  In nonblock mode, don't complain if we're unable to send
   * it all; PQgetResult() will do any additional flushing needed.
   */
#if PG_VERSION_NUM >= 140000
  if (pqPipelineFlush(conn) < 0)
#else
  if (pqFlush(conn) < 0)
#endif
    goto sendFailed;

    /* OK, it's launched! */
#if PG_VERSION_NUM >= 140000
  pqAppendCmdQueueEntry(conn, entry);
#else
  conn->asyncStatus = PGASYNC_BUSY;
#endif
  return 1;

sendFailed:
#if PG_VERSION_NUM >= 140000
  pqRecycleCmdQueueEntry(conn, entry);
#endif
  /* error message should be set up already */
  return 0;
}

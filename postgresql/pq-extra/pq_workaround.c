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
 * This file is mostly a copy-paste from fe-protocol3.c and fe-exec.c
 *
 * The difference is handling `portal suspended` message tag in pqxParseInput3,
 * using this function from PQXgetResult and PQXisBusy.
 *
 * The rest of functions are unfortunately copied as they are static.
 *
 */

#include "pq_workaround.h"
#include "pq_extra_defs.h"

#include <postgres_fe.h>

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include <libpq-int.h>
#include <pg_config.h>

/*
 * This is copy-paste from fe-protocol3.c
 *
 * This macro lists the backend message types that could be "long" (more
 * than a couple of kilobytes).
 */
#define VALID_LONG_MESSAGE_TYPE(id)                                           \
  ((id) == 'T' || (id) == 'D' || (id) == 'd' || (id) == 'V' || (id) == 'E' || \
   (id) == 'N' || (id) == 'A')

static void handleSyncLoss(PGconn* conn, char id, int msgLength);
static int getRowDescriptions(PGconn* conn, int msgLength);
static int getParamDescriptions(PGconn* conn, int msgLength);
static int getAnotherTuple(PGconn* conn, int msgLength);
static int getNotify(PGconn* conn);
static int getCopyStart(PGconn* conn, ExecStatusType copytype);
static int getReadyForQuery(PGconn* conn);

/* Glue to simplify working with error reporting between versions */
#if PG_VERSION_NUM >= 140000
#define updatePQXExpBuffer appendPQExpBuffer
#define updatePQXExpBufferStr appendPQExpBufferStr

#include "pq_pipeline_funcs.i"
#else
#define updatePQXExpBuffer printfPQExpBuffer
#define updatePQXExpBufferStr printfPQExpBuffer
#endif

/*
 * This is a copy-paste of pqSaveWriteError from fe-exec.c
 *
 * As above, after appending conn->write_err_msg to whatever other error we
 * have.  This is used when we've detected a write failure and have exhausted
 * our chances of reporting something else instead.
 */
static void pqSaveWriteError(PGconn* conn) {
#if PG_VERSION_NUM >= 140000
  /*
   * If write_err_msg is null because of previous strdup failure, do what we
   * can.  (It's likely our machinations here will get OOM failures as well,
   * but might as well try.)
   */
  if (conn->write_err_msg) {
    appendPQExpBufferStr(&conn->errorMessage, conn->write_err_msg);
    /* Avoid possibly appending the same message twice */
    conn->write_err_msg[0] = '\0';
  } else
    appendPQExpBufferStr(&conn->errorMessage,
                         libpq_gettext("write to server failed\n"));

  pqSaveErrorResult(conn);
#else
  /*
   * Ensure conn->result is an error result, and add anything in
   * conn->errorMessage to it.
   */
  pqSaveErrorResult(conn);

  /*
   * Now append write_err_msg to that.  If it's null because of previous
   * strdup failure, do what we can.  (It's likely our machinations here are
   * all getting OOM failures as well, but ...)
   */
  if (conn->write_err_msg && conn->write_err_msg[0] != '\0')
    pqCatenateResultError(conn->result, conn->write_err_msg);
  else
    pqCatenateResultError(conn->result,
                          libpq_gettext("write to server failed\n"));
#endif
}

/*
 * This is a copy-paste of getParameterStatus from fe-protocol3.c
 *
 * Attempt to read a ParameterStatus message.
 * This is possible in several places, so we break it out as a subroutine.
 * Entry: 'S' message type and length have already been consumed.
 * Exit: returns 0 if successfully consumed message.
 *     returns EOF if not enough data.
 */
static int getParameterStatus(PGconn* conn) {
  PQExpBufferData valueBuf;

  /* Get the parameter name */
  if (pqGets(&conn->workBuffer, conn)) return EOF;
  /* Get the parameter value (could be large) */
  initPQExpBuffer(&valueBuf);
  if (pqGets(&valueBuf, conn)) {
    termPQExpBuffer(&valueBuf);
    return EOF;
  }
  /* And save it */
  pqSaveParameterStatus(conn, conn->workBuffer.data, valueBuf.data);
  termPQExpBuffer(&valueBuf);
  return 0;
}

/*
 * This is copy-paste of pqParseInput3 from fe-protocol3.c, with added
 * handling of `portal suspended` server message
 *
 * parseInput: if appropriate, parse input data from backend
 * until input is exhausted or a stopping state is reached.
 * Note that this function will NOT attempt to read more data from the
 * backend.
 */
static void pqxParseInput3(PGconn* conn) {
  char id;
  int msgLength;
  int avail;

  /*
   * Loop to parse successive complete messages available in the buffer.
   */
  for (;;) {
    /*
     * Try to read a message.  First get the type code and length. Return
     * if not enough data.
     */
    conn->inCursor = conn->inStart;
    if (pqGetc(&id, conn)) return;
    if (pqGetInt(&msgLength, 4, conn)) return;

    /*
     * Try to validate message type/length here.  A length less than 4 is
     * definitely broken.  Large lengths should only be believed for a few
     * message types.
     */
    if (msgLength < 4) {
      handleSyncLoss(conn, id, msgLength);
      return;
    }
    if (msgLength > 30000 && !VALID_LONG_MESSAGE_TYPE(id)) {
      handleSyncLoss(conn, id, msgLength);
      return;
    }

    /*
     * Can't process if message body isn't all here yet.
     */
    msgLength -= 4;
    avail = conn->inEnd - conn->inCursor;
    if (avail < msgLength) {
      /*
       * Before returning, enlarge the input buffer if needed to hold
       * the whole message.  This is better than leaving it to
       * pqReadData because we can avoid multiple cycles of realloc()
       * when the message is large; also, we can implement a reasonable
       * recovery strategy if we are unable to make the buffer big
       * enough.
       */
      if (pqCheckInBufferSpace(conn->inCursor + (size_t)msgLength, conn)) {
        /*
         * XXX add some better recovery code... plan is to skip over
         * the message using its length, then report an error. For the
         * moment, just treat this like loss of sync (which indeed it
         * might be!)
         */
        handleSyncLoss(conn, id, msgLength);
      }
      return;
    }

    /*
     * NOTIFY and NOTICE messages can happen in any state; always process
     * them right away.
     *
     * Most other messages should only be processed while in BUSY state.
     * (In particular, in READY state we hold off further parsing until
     * the application collects the current PGresult.)
     *
     * However, if the state is IDLE then we got trouble; we need to deal
     * with the unexpected message somehow.
     *
     * ParameterStatus ('S') messages are a special case: in IDLE state we
     * must process 'em (this case could happen if a new value was adopted
     * from config file due to SIGHUP), but otherwise we hold off until
     * BUSY state.
     */
    if (id == 'A') {
      if (getNotify(conn)) return;
    } else if (id == 'N') {
      if (pqGetErrorNotice3(conn, false)) return;
    } else if (conn->asyncStatus != PGASYNC_BUSY) {
      /* If not IDLE state, just wait ... */
      if (conn->asyncStatus != PGASYNC_IDLE) return;

#if PG_VERSION_NUM >= 140000 && PG_VERSION_NUM < 140005
      /*
       * We're also notionally not-IDLE when in pipeline mode the state
       * says "idle" (so we have completed receiving the results of one
       * query from the server and dispatched them to the application)
       * but another query is queued; yield back control to caller so
       * that they can initiate processing of the next query in the
       * queue.
       */
      if (conn->pipelineStatus != PQ_PIPELINE_OFF &&
          conn->cmd_queue_head != NULL)
        return;
#endif

      /*
       * Unexpected message in IDLE state; need to recover somehow.
       * ERROR messages are handled using the notice processor;
       * ParameterStatus is handled normally; anything else is just
       * dropped on the floor after displaying a suitable warning
       * notice.  (An ERROR is very possibly the backend telling us why
       * it is about to close the connection, so we don't want to just
       * discard it...)
       */
      if (id == 'E') {
        if (pqGetErrorNotice3(conn, false /* treat as notice */)) return;
      } else if (id == 'S') {
        if (getParameterStatus(conn)) return;
      } else {
        /* Any other case is unexpected and we summarily skip it */
        pqInternalNotice(&conn->noticeHooks,
                         "message type 0x%02x arrived from server while idle",
                         id);
        /* Discard the unexpected message */
        conn->inCursor += msgLength;
      }
    } else {
      /*
       * In BUSY state, we can process everything.
       */
      switch (id) {
        case 'C': /* command complete */
          if (pqGets(&conn->workBuffer, conn)) return;
          if (conn->result == NULL) {
            conn->result = PQmakeEmptyPGresult(conn, PGRES_COMMAND_OK);
            if (!conn->result) {
              updatePQXExpBufferStr(&conn->errorMessage,
                                    libpq_gettext("out of memory"));
              pqSaveErrorResult(conn);
            }
          }
          if (conn->result)
            strlcpy(conn->result->cmdStatus, conn->workBuffer.data,
                    CMDSTATUS_LEN);
          conn->asyncStatus = PGASYNC_READY;
          break;
        case 'E': /* error return */
          if (pqGetErrorNotice3(conn, true)) return;
          conn->asyncStatus = PGASYNC_READY;
          break;
        case 'Z': /* sync response, backend is ready for new
                   * query */
          if (getReadyForQuery(conn)) return;
#if PG_VERSION_NUM >= 140000
          if (conn->pipelineStatus != PQ_PIPELINE_OFF) {
            conn->result = PQmakeEmptyPGresult(conn, PGRES_PIPELINE_SYNC);
            if (!conn->result) {
              appendPQExpBufferStr(&conn->errorMessage,
                                   libpq_gettext("out of memory"));
              pqSaveErrorResult(conn);
            } else {
              conn->pipelineStatus = PQ_PIPELINE_ON;
              conn->asyncStatus = PGASYNC_READY;
            }
          } else {
            /*
             * In simple query protocol, advance the command queue
             * (see PQgetResult).
             */
            if (conn->cmd_queue_head &&
                conn->cmd_queue_head->queryclass == PGQUERY_SIMPLE)
              pqCommandQueueAdvance(conn);
#else
          {
#endif
            conn->asyncStatus = PGASYNC_IDLE;
          }
          break;
        case 'I': /* empty query */
          if (conn->result == NULL) {
            conn->result = PQmakeEmptyPGresult(conn, PGRES_EMPTY_QUERY);
            if (!conn->result) {
              updatePQXExpBufferStr(&conn->errorMessage,
                                    libpq_gettext("out of memory"));
              pqSaveErrorResult(conn);
            }
          }
          conn->asyncStatus = PGASYNC_READY;
          break;
        case '1': /* Parse Complete */
                  /* If we're doing PQprepare, we're done; else ignore */
#if PG_VERSION_NUM >= 140000
          if (conn->cmd_queue_head &&
              conn->cmd_queue_head->queryclass == PGQUERY_PREPARE) {
#else
          if (conn->queryclass == PGQUERY_PREPARE) {
#endif
            if (conn->result == NULL) {
              conn->result = PQmakeEmptyPGresult(conn, PGRES_COMMAND_OK);
              if (!conn->result) {
                updatePQXExpBufferStr(&conn->errorMessage,
                                      libpq_gettext("out of memory"));
                pqSaveErrorResult(conn);
              }
            }
            conn->asyncStatus = PGASYNC_READY;
          }
          break;
        case '2': /* Bind Complete */
#if PG_VERSION_NUM >= 140000
          if (conn->cmd_queue_head &&
              conn->cmd_queue_head->queryclass == PGXQUERY_BIND) {
            pqCommandQueueAdvance(conn);
#else
          if (conn->queryclass == PGXQUERY_BIND) {
#endif
            /* In case of portal bind, the query ends here without a result */
#if PG_VERSION_NUM >= 140005
            if (conn->pipelineStatus != PQ_PIPELINE_OFF)
              conn->asyncStatus = PGASYNC_PIPELINE_IDLE;
            else
#endif
              conn->asyncStatus = PGASYNC_IDLE;
          }
          break;
        case '3': /* Close Complete */
#if PG_VERSION_NUM >= 140005
          /*
           * If we get CloseComplete when waiting for it, consume
           * the queue element and keep going.  A result is not
           * expected from this message; it is just there so that
           * we know to wait for it when PQsendQuery is used in
           * pipeline mode, before going in IDLE state.  Failing to
           * do this makes us receive CloseComplete when IDLE, which
           * creates problems.
           */
          if (conn->cmd_queue_head &&
              conn->cmd_queue_head->queryclass == PGQUERY_CLOSE) {
            pqCommandQueueAdvance(conn);
          }
#endif
          break;
        case 'S': /* parameter status */
          if (getParameterStatus(conn)) return;
          break;
        case 'K': /* secret key data from the backend */

          /*
           * This is expected only during backend startup, but it's
           * just as easy to handle it as part of the main loop.
           * Save the data and continue processing.
           */
          if (pqGetInt(&(conn->be_pid), 4, conn)) return;
          if (pqGetInt(&(conn->be_key), 4, conn)) return;
          break;
        case 'T': /* Row Description */
          if (conn->result != NULL &&
              conn->result->resultStatus == PGRES_FATAL_ERROR) {
            /*
             * We've already choked for some reason.  Just discard
             * the data till we get to the end of the query.
             */
            conn->inCursor += msgLength;
          } else if (conn->result == NULL ||
#if PG_VERSION_NUM >= 140000
                     (conn->cmd_queue_head &&
                      conn->cmd_queue_head->queryclass == PGQUERY_DESCRIBE)) {
#else
                     conn->queryclass == PGQUERY_DESCRIBE) {
#endif
            /* First 'T' in a query sequence */
            if (getRowDescriptions(conn, msgLength)) return;
          } else {
            /*
             * A new 'T' message is treated as the start of
             * another PGresult.  (It is not clear that this is
             * really possible with the current backend.) We stop
             * parsing until the application accepts the current
             * result.
             */
            conn->asyncStatus = PGASYNC_READY;
            return;
          }
          break;
        case 'n': /* No Data */

          /*
           * NoData indicates that we will not be seeing a
           * RowDescription message because the statement or portal
           * inquired about doesn't return rows.
           *
           * If we're doing a Describe, we have to pass something
           * back to the client, so set up a COMMAND_OK result,
           * instead of PGRES_TUPLES_OK.  Otherwise we can just
           * ignore this message.
           */
#if PG_VERSION_NUM >= 140000
          if (conn->cmd_queue_head &&
              conn->cmd_queue_head->queryclass == PGQUERY_DESCRIBE) {
#else
          if (conn->queryclass == PGQUERY_DESCRIBE) {
#endif
            if (conn->result == NULL) {
              conn->result = PQmakeEmptyPGresult(conn, PGRES_COMMAND_OK);
              if (!conn->result) {
                updatePQXExpBufferStr(&conn->errorMessage,
                                      libpq_gettext("out of memory"));
                pqSaveErrorResult(conn);
              }
            }
            conn->asyncStatus = PGASYNC_READY;
          }
          break;
        case 't': /* Parameter Description */
          if (getParamDescriptions(conn, msgLength)) return;
          break;
        case 'D': /* Data Row */
          if (conn->result != NULL &&
              conn->result->resultStatus == PGRES_TUPLES_OK) {
            /* Read another tuple of a normal query response */
            if (getAnotherTuple(conn, msgLength)) return;
          } else if (conn->result != NULL &&
                     conn->result->resultStatus == PGRES_FATAL_ERROR) {
            /*
             * We've already choked for some reason.  Just discard
             * tuples till we get to the end of the query.
             */
            conn->inCursor += msgLength;
          } else {
            /* Set up to report error at end of query */
            updatePQXExpBufferStr(
                &conn->errorMessage,
                libpq_gettext("server sent data (\"D\" message) without prior "
                              "row description (\"T\" message)\n"));
            pqSaveErrorResult(conn);
            /* Discard the unexpected message */
            conn->inCursor += msgLength;
          }
          break;
        case 'G': /* Start Copy In */
          if (getCopyStart(conn, PGRES_COPY_IN)) return;
          conn->asyncStatus = PGASYNC_COPY_IN;
          break;
        case 'H': /* Start Copy Out */
          if (getCopyStart(conn, PGRES_COPY_OUT)) return;
          conn->asyncStatus = PGASYNC_COPY_OUT;
          conn->copy_already_done = 0;
          break;
        case 'W': /* Start Copy Both */
          if (getCopyStart(conn, PGRES_COPY_BOTH)) return;
          conn->asyncStatus = PGASYNC_COPY_BOTH;
          conn->copy_already_done = 0;
          break;
        case 'd': /* Copy Data */

          /*
           * If we see Copy Data, just silently drop it.  This would
           * only occur if application exits COPY OUT mode too
           * early.
           */
          conn->inCursor += msgLength;
          break;
        case 'c': /* Copy Done */

          /*
           * If we see Copy Done, just silently drop it.  This is
           * the normal case during PQendcopy.  We will keep
           * swallowing data, expecting to see command-complete for
           * the COPY command.
           */
          break;
        case 's': /* Portal Suspended */
          /*
           * We see this message only when an application issued an
           * execute portal command with row limit. We can effectively
           * ignore the message
           */
          conn->asyncStatus = PGASYNC_READY;
          break;
        default:
          updatePQXExpBuffer(
              &conn->errorMessage,
              libpq_gettext("unexpected response from server; first received "
                            "character was \"%c\"\n"),
              id);
          /* build an error result holding the error message */
          pqSaveErrorResult(conn);
          /* not sure if we will see more, so go to ready state */
          conn->asyncStatus = PGASYNC_READY;
          /* Discard the unexpected message */
          conn->inCursor += msgLength;
          break;
      } /* switch on protocol character */
    }
    /* Successfully consumed this message */
    if (conn->inCursor == conn->inStart + 5 + msgLength) {
#if PG_VERSION_NUM >= 140000
      /* trace server-to-client message */
      if (conn->Pfdebug)
        pqTraceOutputMessage(conn, conn->inBuffer + conn->inStart, false);
#endif
      /* Normal case: parsing agrees with specified length */
      conn->inStart = conn->inCursor;
    } else {
      /* Trouble --- report it */
      updatePQXExpBuffer(&conn->errorMessage,
                         libpq_gettext("message contents do not agree with "
                                       "length in message type \"%c\"\n"),
                         id);
      /* build an error result holding the error message */
      pqSaveErrorResult(conn);
      conn->asyncStatus = PGASYNC_READY;
      /* trust the specified message length as what to skip */
      conn->inStart += 5 + msgLength;
    }
  }
}

/*
 * This is copy-paste of libpq's parseInput from fe-exec.c using fixed
 * pqxParseInput3
 *
 * parseInput: if appropriate, parse input data from backend
 * until input is exhausted or a stopping state is reached.
 * Note that this function will NOT attempt to read more data from the
 * backend.
 */
static void parseInput(PGconn* conn) {
#if PG_VERSION_NUM >= 140000
  pqxParseInput3(conn);
#else
  if (PG_PROTOCOL_MAJOR(conn->pversion) >= 3)
    pqxParseInput3(conn);
  else
    /* For compatibility - parse protocol v2, very infeasible */
    pqParseInput2(conn);
#endif
}

/*
 * getCopyResult
 *
 *    This is a copy-paste of original getCopyResult, as far as it's
 *    static in fe-exec.c
 *
 *    Helper for PQgetResult: generate result for COPY-in-progress cases
 */
static PGresult* getCopyResult(PGconn* conn, ExecStatusType copytype) {
  /*
   * If the server connection has been lost, don't pretend everything is
   * hunky-dory; instead return a PGRES_FATAL_ERROR result, and reset the
   * asyncStatus to idle (corresponding to what we'd do if we'd detected I/O
   * error in the earlier steps in PQgetResult).  The text returned in the
   * result is whatever is in conn->errorMessage; we hope that was filled
   * with something relevant when the lost connection was detected.
   */
  if (conn->status != CONNECTION_OK) {
    pqSaveErrorResult(conn);
    conn->asyncStatus = PGASYNC_IDLE;
    return pqPrepareAsyncResult(conn);
  }

  /* If we have an async result for the COPY, return that */
  if (conn->result && conn->result->resultStatus == copytype)
    return pqPrepareAsyncResult(conn);

  /* Otherwise, invent a suitable PGresult */
  return PQmakeEmptyPGresult(conn, copytype);
}

/*
 * PXQisBusy
 *
 *   This is a copy-paste of original PQisBusy from fe-exec.c, needed to use
 *   patched version of parseInput
 *
 *   Return true if PQgetResult would block waiting for input.
 */

int PQXisBusy(PGconn* conn) {
  if (!conn) return false;

  /* Parse any available data, if our state permits. */
  parseInput(conn);

  /*
   * PQgetResult will return immediately in all states except BUSY, or if we
   * had a write failure.
   */
  return conn->asyncStatus == PGASYNC_BUSY || conn->write_failed;
}

/*
 * PQXgetResult
 *
 *    This is a copy-paste of original PQgetResult from fe-exec.c, needed to use
 *    patched version of parseInput
 *
 *    Get the next PGresult produced by a query.  Returns NULL if no
 *    query work remains or an error has occurred (e.g. out of
 *    memory).
 *
 *    In pipeline mode, once all the result of a query have been returned,
 *    PQgetResult returns NULL to let the user know that the next
 *    query is being processed.  At the end of the pipeline, returns a
 *    result with PQresultStatus(result) == PGRES_PIPELINE_SYNC.
 */
PGresult* PQXgetResult(PGconn* conn) {
  if (!conn) return NULL;

  PGresult* res = NULL;

  /* Parse any available data, if our state permits. */
  parseInput(conn);

  /* If not ready to return something, block until we are. */
  while (conn->asyncStatus == PGASYNC_BUSY) {
    int flushResult;

    /*
     * If data remains unsent, send it.  Else we might be waiting for the
     * result of a command the backend hasn't even got yet.
     */
    while ((flushResult = pqFlush(conn)) > 0) {
      if (pqWait(false, true, conn)) {
        flushResult = -1;
        break;
      }
    }

    /*
     * Wait for some more data, and load it.  (Note: if the connection has
     * been lost, pqWait should return immediately because the socket
     * should be read-ready, either with the last server data or with an
     * EOF indication.  We expect therefore that this won't result in any
     * undue delay in reporting a previous write failure.)
     */
    if (flushResult || pqWait(true, false, conn) || pqReadData(conn) < 0) {
      /*
       * conn->errorMessage has been set by pqWait or pqReadData. We
       * want to append it to any already-received error message.
       */
      pqSaveErrorResult(conn);
      conn->asyncStatus = PGASYNC_IDLE;
      return pqPrepareAsyncResult(conn);
    }

    /* Parse it. */
    parseInput(conn);

    /*
     * If we had a write error, but nothing above obtained a query result
     * or detected a read error, report the write error.
     */
    if (conn->write_failed && conn->asyncStatus == PGASYNC_BUSY) {
      pqSaveWriteError(conn);
      conn->asyncStatus = PGASYNC_IDLE;
      return pqPrepareAsyncResult(conn);
    }
  }

  /* Return the appropriate thing. */
  switch (conn->asyncStatus) {
    case PGASYNC_IDLE:
      res = NULL; /* query is complete */
#if PG_VERSION_NUM >= 140000 && PG_VERSION_NUM < 140005
      if (conn->pipelineStatus != PQ_PIPELINE_OFF) {
        /*
         * We're about to return the NULL that terminates the round of
         * results from the current query; prepare to send the results
         * of the next query when we're called next.  Also, since this
         * is the start of the results of the next query, clear any
         * prior error message.
         */
        resetPQExpBuffer(&conn->errorMessage);
        pqPipelineProcessQueue(conn);
      }
#endif
      break;
#if PG_VERSION_NUM >= 140005
    case PGASYNC_PIPELINE_IDLE:
      Assert(conn->pipelineStatus != PQ_PIPELINE_OFF);

      /*
       * We're about to return the NULL that terminates the round of
       * results from the current query; prepare to send the results
       * of the next query, if any, when we're called next.  If there's
       * no next element in the command queue, this gets us in IDLE
       * state.
       */
      resetPQExpBuffer(&conn->errorMessage);
      pqPipelineProcessQueue(conn);
      res = NULL; /* query is complete */
      break;
#endif
    case PGASYNC_READY:
#if PG_VERSION_NUM >= 140000
      /*
       * For any query type other than simple query protocol, we advance
       * the command queue here.  This is because for simple query
       * protocol we can get the READY state multiple times before the
       * command is actually complete, since the command string can
       * contain many queries.  In simple query protocol, the queue
       * advance is done by fe-protocol3 when it receives ReadyForQuery.
       */
      if (conn->cmd_queue_head &&
          conn->cmd_queue_head->queryclass != PGQUERY_SIMPLE)
        pqCommandQueueAdvance(conn);
      res = pqPrepareAsyncResult(conn);
      if (conn->pipelineStatus != PQ_PIPELINE_OFF) {
        /*
         * We're about to send the results of the current query.  Set
         * us idle now, and ...
         */
#if PG_VERSION_NUM < 140005
        conn->asyncStatus = PGASYNC_IDLE;
#else
        conn->asyncStatus = PGASYNC_PIPELINE_IDLE;
#endif

        /*
         * ... in cases when we're sending a pipeline-sync result,
         * move queue processing forwards immediately, so that next
         * time we're called, we're prepared to return the next result
         * received from the server.  In all other cases, leave the
         * queue state change for next time, so that a terminating
         * NULL result is sent.
         *
         * (In other words: we don't return a NULL after a pipeline
         * sync.)
         */
        if (res && res->resultStatus == PGRES_PIPELINE_SYNC)
          pqPipelineProcessQueue(conn);
      } else {
        /* Set the state back to BUSY, allowing parsing to proceed. */
        conn->asyncStatus = PGASYNC_BUSY;
      }
      break;
    case PGASYNC_READY_MORE:
#endif
      res = pqPrepareAsyncResult(conn);
      /* Set the state back to BUSY, allowing parsing to proceed. */
      conn->asyncStatus = PGASYNC_BUSY;
      break;
    case PGASYNC_COPY_IN:
      res = getCopyResult(conn, PGRES_COPY_IN);
      break;
    case PGASYNC_COPY_OUT:
      res = getCopyResult(conn, PGRES_COPY_OUT);
      break;
    case PGASYNC_COPY_BOTH:
      res = getCopyResult(conn, PGRES_COPY_BOTH);
      break;
    default:
      updatePQXExpBuffer(&conn->errorMessage,
                         libpq_gettext("unexpected asyncStatus: %d\n"),
                         (int)conn->asyncStatus);
      res = PQmakeEmptyPGresult(conn, PGRES_FATAL_ERROR);
      break;
  }

#if PG_VERSION_NUM >= 140005
  /* If the next command we expect is CLOSE, read and consume it */
  if (conn->asyncStatus == PGASYNC_PIPELINE_IDLE && conn->cmd_queue_head &&
      conn->cmd_queue_head->queryclass == PGQUERY_CLOSE) {
    if (res && res->resultStatus != PGRES_FATAL_ERROR) {
      conn->asyncStatus = PGASYNC_BUSY;
      parseInput(conn);
      conn->asyncStatus = PGASYNC_PIPELINE_IDLE;
    } else
      /* we won't ever see the Close */
      pqCommandQueueAdvance(conn);
  }
#endif

  if (res) {
    int i;

    for (i = 0; i < res->nEvents; i++) {
      PGEventResultCreate evt;

      evt.conn = conn;
      evt.result = res;
      if (!res->events[i].proc(PGEVT_RESULTCREATE, &evt,
                               res->events[i].passThrough)) {
        updatePQXExpBuffer(
            &conn->errorMessage,
            libpq_gettext(
                "PGEventProc \"%s\" failed during PGEVT_RESULTCREATE event\n"),
            res->events[i].name);
#if PG_VERSION_NUM >= 150000
        // We might use `conn->errorReported` instead of a 0
        // to negate a rare possibility of messages duplication
        pqSetResultError(res, &conn->errorMessage, 0);
#elif PG_VERSION_NUM >= 140000
        pqSetResultError(res, &conn->errorMessage);
#else
        pqSetResultError(res, conn->errorMessage.data);
#endif
        res->resultStatus = PGRES_FATAL_ERROR;
        break;
      }
      res->events[i].resultInitialized = true;
    }
  }

  return res;
}

/*
 * This is copy-paste of handleSyncLoss from fe-protocol3.c
 *
 * handleSyncLoss: clean up after loss of message-boundary sync
 *
 * There isn't really a lot we can do here except abandon the connection.
 */
static void handleSyncLoss(PGconn* conn, char id, int msgLength) {
  updatePQXExpBuffer(&conn->errorMessage,
                     libpq_gettext("lost synchronization with server: got "
                                   "message type \"%c\", length %d\n"),
                     id, msgLength);
  /* build an error result holding the error message */
  pqSaveErrorResult(conn);
  conn->asyncStatus = PGASYNC_READY; /* drop out of PQgetResult wait loop */
  /* flush input data since we're giving up on processing it */
  pqDropConnection(conn, true);
  conn->status = CONNECTION_BAD; /* No more connection to backend */
}

/*
 * This is copy-paste of getReadyForQuery from fe-protocol3.c
 *
 * getReadyForQuery - process ReadyForQuery message
 */
static int getReadyForQuery(PGconn* conn) {
  char xact_status;

  if (pqGetc(&xact_status, conn)) return EOF;
  switch (xact_status) {
    case 'I':
      conn->xactStatus = PQTRANS_IDLE;
      break;
    case 'T':
      conn->xactStatus = PQTRANS_INTRANS;
      break;
    case 'E':
      conn->xactStatus = PQTRANS_INERROR;
      break;
    default:
      conn->xactStatus = PQTRANS_UNKNOWN;
      break;
  }

  return 0;
}

/*
 * This is copy-paste of getNotify from fe-protocol3.c
 *
 * Attempt to read a Notify response message.
 * This is possible in several places, so we break it out as a subroutine.
 * Entry: 'A' message type and length have already been consumed.
 * Exit: returns 0 if successfully consumed Notify message.
 *     returns EOF if not enough data.
 */
static int getNotify(PGconn* conn) {
  int be_pid;
  char* svname;
  int nmlen;
  int extralen;
  PGnotify* newNotify;

  if (pqGetInt(&be_pid, 4, conn)) return EOF;
  if (pqGets(&conn->workBuffer, conn)) return EOF;
  /* must save name while getting extra string */
  svname = strdup(conn->workBuffer.data);
  if (!svname) return EOF;
  if (pqGets(&conn->workBuffer, conn)) {
    free(svname);
    return EOF;
  }

  /*
   * Store the strings right after the PQnotify structure so it can all be
   * freed at once.  We don't use NAMEDATALEN because we don't want to tie
   * this interface to a specific server name length.
   */
  nmlen = strnlen(svname, INT_MAX);
  extralen = strnlen(conn->workBuffer.data, INT_MAX);
  newNotify = (PGnotify*)malloc(sizeof(PGnotify) + nmlen + extralen + 2);
  if (newNotify) {
    newNotify->relname = (char*)newNotify + sizeof(PGnotify);
    strncpy(newNotify->relname, svname, nmlen + 1);
    newNotify->extra = newNotify->relname + nmlen + 1;
    strncpy(newNotify->extra, conn->workBuffer.data, extralen + 1);
    newNotify->be_pid = be_pid;
    newNotify->next = NULL;
    if (conn->notifyTail)
      conn->notifyTail->next = newNotify;
    else
      conn->notifyHead = newNotify;
    conn->notifyTail = newNotify;
  }

  free(svname);
  return 0;
}

/*
 * This is copy-paste of getRowDescriptions from fe-protocol3.c
 *
 * parseInput subroutine to read a 'T' (row descriptions) message.
 * We'll build a new PGresult structure (unless called for a Describe
 * command for a prepared statement) containing the attribute data.
 * Returns: 0 if processed message successfully, EOF to suspend parsing
 * (the latter case is not actually used currently).
 * In the former case, conn->inStart has been advanced past the message.
 */
static int getRowDescriptions(PGconn* conn, int msgLength) {
  PGresult* result;
  int nfields;
  const char* errmsg;
  int i;

  /*
   * When doing Describe for a prepared statement, there'll already be a
   * PGresult created by getParamDescriptions, and we should fill data into
   * that.  Otherwise, create a new, empty PGresult.
   */
#if PG_VERSION_NUM >= 140000
  if (!conn->cmd_queue_head ||
      (conn->cmd_queue_head &&
       conn->cmd_queue_head->queryclass == PGQUERY_DESCRIBE)) {
#else
  if (conn->queryclass == PGQUERY_DESCRIBE) {
#endif
    if (conn->result)
      result = conn->result;
    else
      result = PQmakeEmptyPGresult(conn, PGRES_COMMAND_OK);
  } else
    result = PQmakeEmptyPGresult(conn, PGRES_TUPLES_OK);
  if (!result) {
    errmsg = NULL; /* means "out of memory", see below */
    goto advance_and_error;
  }

  /* parseInput already read the 'T' label and message length. */
  /* the next two bytes are the number of fields */
  if (pqGetInt(&(result->numAttributes), 2, conn)) {
    /* We should not run out of data here, so complain */
    errmsg = libpq_gettext("insufficient data in \"T\" message");
    goto advance_and_error;
  }
  nfields = result->numAttributes;

  /* allocate space for the attribute descriptors */
  if (nfields > 0) {
    result->attDescs = (PGresAttDesc*)pqResultAlloc(
        result, nfields * sizeof(PGresAttDesc), true);
    if (!result->attDescs) {
      errmsg = NULL; /* means "out of memory", see below */
      goto advance_and_error;
    }
    MemSet(result->attDescs, 0, nfields * sizeof(PGresAttDesc));
  }

  /* result->binary is true only if ALL columns are binary */
  result->binary = (nfields > 0) ? 1 : 0;

  /* get type info */
  for (i = 0; i < nfields; i++) {
    int tableid;
    int columnid;
    int typid;
    int typlen;
    int atttypmod;
    int format;

    if (pqGets(&conn->workBuffer, conn) || pqGetInt(&tableid, 4, conn) ||
        pqGetInt(&columnid, 2, conn) || pqGetInt(&typid, 4, conn) ||
        pqGetInt(&typlen, 2, conn) || pqGetInt(&atttypmod, 4, conn) ||
        pqGetInt(&format, 2, conn)) {
      /* We should not run out of data here, so complain */
      errmsg = libpq_gettext("insufficient data in \"T\" message");
      goto advance_and_error;
    }

    /*
     * Since pqGetInt treats 2-byte integers as unsigned, we need to
     * coerce these results to signed form.
     */
    columnid = (int)((int16)columnid);
    typlen = (int)((int16)typlen);
    format = (int)((int16)format);

    result->attDescs[i].name = pqResultStrdup(result, conn->workBuffer.data);
    if (!result->attDescs[i].name) {
      errmsg = NULL; /* means "out of memory", see below */
      goto advance_and_error;
    }
    result->attDescs[i].tableid = tableid;
    result->attDescs[i].columnid = columnid;
    result->attDescs[i].format = format;
    result->attDescs[i].typid = typid;
    result->attDescs[i].typlen = typlen;
    result->attDescs[i].atttypmod = atttypmod;

    if (format != 1) result->binary = 0;
  }

  /* Success! */
  conn->result = result;

  /*
   * If we're doing a Describe, we're done, and ready to pass the result
   * back to the client.
   */
#if PG_VERSION_NUM >= 140000
  if ((!conn->cmd_queue_head) ||
      (conn->cmd_queue_head &&
       conn->cmd_queue_head->queryclass == PGQUERY_DESCRIBE)) {
#else
  if (conn->queryclass == PGQUERY_DESCRIBE) {
#endif
    conn->asyncStatus = PGASYNC_READY;
    return 0;
  }

  /*
   * We could perform additional setup for the new result set here, but for
   * now there's nothing else to do.
   */

  /* And we're done. */
  return 0;

advance_and_error:
  /* Discard unsaved result, if any */
  if (result && result != conn->result) PQclear(result);

  /*
   * Replace partially constructed result with an error result. First
   * discard the old result to try to win back some memory.
   */
  pqClearAsyncResult(conn);

  /*
   * If preceding code didn't provide an error message, assume "out of
   * memory" was meant.  The advantage of having this special case is that
   * freeing the old result first greatly improves the odds that gettext()
   * will succeed in providing a translation.
   */
  if (!errmsg) errmsg = libpq_gettext("out of memory for query result");

  updatePQXExpBuffer(&conn->errorMessage, "%s\n", errmsg);
  pqSaveErrorResult(conn);

  /*
   * Show the message as fully consumed, else pqParseInput3 will overwrite
   * our error with a complaint about that.
   */
  conn->inCursor = conn->inStart + 5 + msgLength;

  /*
   * Return zero to allow input parsing to continue.  Subsequent "D"
   * messages will be ignored until we get to end of data, since an error
   * result is already set up.
   */
  return 0;
}

/*
 * This is copy-paste of getParamDescriptions from fe-protocol3.c
 *
 * parseInput subroutine to read a 't' (ParameterDescription) message.
 * We'll build a new PGresult structure containing the parameter data.
 * Returns: 0 if processed message successfully, EOF to suspend parsing
 * (the latter case is not actually used currently).
 */
static int getParamDescriptions(PGconn* conn, int msgLength) {
  PGresult* result;
  const char* errmsg = NULL; /* means "out of memory", see below */
  int nparams;
  int i;

  result = PQmakeEmptyPGresult(conn, PGRES_COMMAND_OK);
  if (!result) goto advance_and_error;

  /* parseInput already read the 't' label and message length. */
  /* the next two bytes are the number of parameters */
  if (pqGetInt(&(result->numParameters), 2, conn)) goto not_enough_data;
  nparams = result->numParameters;

  /* allocate space for the parameter descriptors */
  if (nparams > 0) {
    result->paramDescs = (PGresParamDesc*)pqResultAlloc(
        result, nparams * sizeof(PGresParamDesc), true);
    if (!result->paramDescs) goto advance_and_error;
    MemSet(result->paramDescs, 0, nparams * sizeof(PGresParamDesc));
  }

  /* get parameter info */
  for (i = 0; i < nparams; i++) {
    int typid;

    if (pqGetInt(&typid, 4, conn)) goto not_enough_data;
    result->paramDescs[i].typid = typid;
  }

  /* Success! */
  conn->result = result;

  return 0;

not_enough_data:
  errmsg = libpq_gettext("insufficient data in \"t\" message");

advance_and_error:
  /* Discard unsaved result, if any */
  if (result && result != conn->result) PQclear(result);

  /*
   * Replace partially constructed result with an error result. First
   * discard the old result to try to win back some memory.
   */
  pqClearAsyncResult(conn);

  /*
   * If preceding code didn't provide an error message, assume "out of
   * memory" was meant.  The advantage of having this special case is that
   * freeing the old result first greatly improves the odds that gettext()
   * will succeed in providing a translation.
   */
  if (!errmsg) errmsg = libpq_gettext("out of memory");
  updatePQXExpBuffer(&conn->errorMessage, "%s\n", errmsg);
  pqSaveErrorResult(conn);

  /*
   * Show the message as fully consumed, else pqParseInput3 will overwrite
   * our error with a complaint about that.
   */
  conn->inCursor = conn->inStart + 5 + msgLength;

  /*
   * Return zero to allow input parsing to continue.  Essentially, we've
   * replaced the COMMAND_OK result with an error result, but since this
   * doesn't affect the protocol state, it's fine.
   */
  return 0;
}

/*
 * This is copy-paste of getAnotherTuple from fe-protocol3.c
 *
 * parseInput subroutine to read a 'D' (row data) message.
 * We fill rowbuf with column pointers and then call the row processor.
 * Returns: 0 if processed message successfully, EOF to suspend parsing
 * (the latter case is not actually used currently).
 */
static int getAnotherTuple(PGconn* conn, int msgLength) {
  PGresult* result = conn->result;
  int nfields = result->numAttributes;
  const char* errmsg;
  PGdataValue* rowbuf;
  int tupnfields; /* # fields from tuple */
  int vlen;       /* length of the current field value */
  int i;

  /* Get the field count and make sure it's what we expect */
  if (pqGetInt(&tupnfields, 2, conn)) {
    /* We should not run out of data here, so complain */
    errmsg = libpq_gettext("insufficient data in \"D\" message");
    goto advance_and_error;
  }

  if (tupnfields != nfields) {
    errmsg = libpq_gettext("unexpected field count in \"D\" message");
    goto advance_and_error;
  }

  /* Resize row buffer if needed */
  rowbuf = conn->rowBuf;
  if (nfields > conn->rowBufLen) {
    rowbuf = (PGdataValue*)realloc(rowbuf, nfields * sizeof(PGdataValue));
    if (!rowbuf) {
      errmsg = NULL; /* means "out of memory", see below */
      goto advance_and_error;
    }
    conn->rowBuf = rowbuf;
    conn->rowBufLen = nfields;
  }

  /* Scan the fields */
  for (i = 0; i < nfields; i++) {
    /* get the value length */
    if (pqGetInt(&vlen, 4, conn)) {
      /* We should not run out of data here, so complain */
      errmsg = libpq_gettext("insufficient data in \"D\" message");
      goto advance_and_error;
    }
    rowbuf[i].len = vlen;

    /*
     * rowbuf[i].value always points to the next address in the data
     * buffer even if the value is NULL.  This allows row processors to
     * estimate data sizes more easily.
     */
    rowbuf[i].value = conn->inBuffer + conn->inCursor;

    /* Skip over the data value */
    if (vlen > 0) {
      if (pqSkipnchar(vlen, conn)) {
        /* We should not run out of data here, so complain */
        errmsg = libpq_gettext("insufficient data in \"D\" message");
        goto advance_and_error;
      }
    }
  }

  /* Process the collected row */
  errmsg = NULL;
  if (pqRowProcessor(conn, &errmsg)) return 0; /* normal, successful exit */

  /* pqRowProcessor failed, fall through to report it */

advance_and_error:

  /*
   * Replace partially constructed result with an error result. First
   * discard the old result to try to win back some memory.
   */
  pqClearAsyncResult(conn);

  /*
   * If preceding code didn't provide an error message, assume "out of
   * memory" was meant.  The advantage of having this special case is that
   * freeing the old result first greatly improves the odds that gettext()
   * will succeed in providing a translation.
   */
  if (!errmsg) errmsg = libpq_gettext("out of memory for query result");

  updatePQXExpBuffer(&conn->errorMessage, "%s\n", errmsg);
  pqSaveErrorResult(conn);

  /*
   * Show the message as fully consumed, else pqParseInput3 will overwrite
   * our error with a complaint about that.
   */
  conn->inCursor = conn->inStart + 5 + msgLength;

  /*
   * Return zero to allow input parsing to continue.  Subsequent "D"
   * messages will be ignored until we get to end of data, since an error
   * result is already set up.
   */
  return 0;
}

/*
 * This is copy-paste of getCopyStart from fe-protocol3.c
 *
 * getCopyStart - process CopyInResponse, CopyOutResponse or
 * CopyBothResponse message
 *
 * parseInput already read the message type and length.
 */
static int getCopyStart(PGconn* conn, ExecStatusType copytype) {
  PGresult* result;
  int nfields;
  int i;

  result = PQmakeEmptyPGresult(conn, copytype);
  if (!result) goto failure;

  if (pqGetc(&conn->copy_is_binary, conn)) goto failure;
  result->binary = conn->copy_is_binary;
  /* the next two bytes are the number of fields  */
  if (pqGetInt(&(result->numAttributes), 2, conn)) goto failure;
  nfields = result->numAttributes;

  /* allocate space for the attribute descriptors */
  if (nfields > 0) {
    result->attDescs = (PGresAttDesc*)pqResultAlloc(
        result, nfields * sizeof(PGresAttDesc), true);
    if (!result->attDescs) goto failure;
    MemSet(result->attDescs, 0, nfields * sizeof(PGresAttDesc));
  }

  for (i = 0; i < nfields; i++) {
    int format;

    if (pqGetInt(&format, 2, conn)) goto failure;

    /*
     * Since pqGetInt treats 2-byte integers as unsigned, we need to
     * coerce these results to signed form.
     */
    format = (int)((int16)format);
    result->attDescs[i].format = format;
  }

  /* Success! */
  conn->result = result;
  return 0;

failure:
  PQclear(result);
  return EOF;
}

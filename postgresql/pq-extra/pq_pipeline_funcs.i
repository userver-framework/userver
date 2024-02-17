#if PG_VERSION_NUM < 140000
#error This file is for postgres versions 14.0+
#endif

/*
 * pqPipelineProcessQueue: subroutine for PQgetResult
 *		In pipeline mode, start processing the results of the next query
 *    in the queue.
 *
 * This is a copy-paste of pqPipelineProcessQueue from fe-exec.c
 */
static void pqPipelineProcessQueue(PGconn* conn) {
  switch (conn->asyncStatus) {
    case PGASYNC_COPY_IN:
    case PGASYNC_COPY_OUT:
    case PGASYNC_COPY_BOTH:
    case PGASYNC_READY:
    case PGASYNC_READY_MORE:
    case PGASYNC_BUSY:
      /* client still has to process current query or results */
      return;
    case PGASYNC_IDLE:
#if PG_VERSION_NUM >= 140005
      /*
       * If we're in IDLE mode and there's some command in the queue,
       * get us into PIPELINE_IDLE mode and process normally.  Otherwise
       * there's nothing for us to do.
       */
      if (conn->cmd_queue_head != NULL) {
        conn->asyncStatus = PGASYNC_PIPELINE_IDLE;
        break;
      }
      return;

    case PGASYNC_PIPELINE_IDLE:
      Assert(conn->pipelineStatus != PQ_PIPELINE_OFF);
#endif
      /* next query please */
      break;
  }

#if PG_VERSION_NUM < 140005
  /* Nothing to do if not in pipeline mode, or queue is empty */
  if (conn->pipelineStatus == PQ_PIPELINE_OFF || conn->cmd_queue_head == NULL)
    return;
#else
  /*
   * If there are no further commands to process in the queue, get us in
   * "real idle" mode now.
   */
  if (conn->cmd_queue_head == NULL) {
    conn->asyncStatus = PGASYNC_IDLE;
    return;
  }
#endif

  /* Initialize async result-accumulation state */
  pqClearAsyncResult(conn);

  /*
   * Reset single-row processing mode.  (Client has to set it up for each
   * query, if desired.)
   */
  conn->singleRowMode = false;

  if (conn->pipelineStatus == PQ_PIPELINE_ABORTED &&
      conn->cmd_queue_head->queryclass != PGQUERY_SYNC) {
    /*
     * In an aborted pipeline we don't get anything from the server for
     * each result; we're just discarding commands from the queue until we
     * get to the next sync from the server.
     *
     * The PGRES_PIPELINE_ABORTED results tell the client that its queries
     * got aborted.
     */
    conn->result = PQmakeEmptyPGresult(conn, PGRES_PIPELINE_ABORTED);
    if (!conn->result) {
      appendPQExpBufferStr(&conn->errorMessage,
                           libpq_gettext("out of memory\n"));
      pqSaveErrorResult(conn);
      return;
    }
    conn->asyncStatus = PGASYNC_READY;
  } else {
    /* allow parsing to continue */
    conn->asyncStatus = PGASYNC_BUSY;
  }
}

/*
 * pqPipelineFlush
 *
 * This is a copy-paste of pqPipelineFlush from fe-exec.c
 *
 * In pipeline mode, data will be flushed only when the out buffer reaches the
 * threshold value.  In non-pipeline mode, it behaves as stock pqFlush.
 *
 * Returns 0 on success.
 */
__attribute__((unused)) static int pqPipelineFlush(PGconn* conn) {
  if ((conn->pipelineStatus != PQ_PIPELINE_ON) ||
      (conn->outCount >= OUTBUFFER_THRESHOLD))
    return pqFlush(conn);
  return 0;
}

/*
 * pqAllocCmdQueueEntry
 *		Get a command queue entry for caller to fill.
 *
 * This is a copy-paste of pqAllocCmdQueueEntry from fe-exec.c
 *
 * If the recycle queue has a free element, that is returned; if not, a
 * fresh one is allocated.  Caller is responsible for adding it to the
 * command queue (pqAppendCmdQueueEntry) once the struct is filled in, or
 * releasing the memory (pqRecycleCmdQueueEntry) if an error occurs.
 *
 * If allocation fails, sets the error message and returns NULL.
 */
__attribute__((unused)) static PGcmdQueueEntry* pqAllocCmdQueueEntry(
    PGconn* conn) {
  PGcmdQueueEntry* entry;

  if (conn->cmd_queue_recycle == NULL) {
    entry = (PGcmdQueueEntry*)malloc(sizeof(PGcmdQueueEntry));
    if (entry == NULL) {
      appendPQExpBufferStr(&conn->errorMessage,
                           libpq_gettext("out of memory\n"));
      return NULL;
    }
  } else {
    entry = conn->cmd_queue_recycle;
    conn->cmd_queue_recycle = entry->next;
  }
  entry->next = NULL;
  entry->query = NULL;

  return entry;
}

/*
 * pqAppendCmdQueueEntry
 *		Append a caller-allocated entry to the command queue, and update
 *		conn->asyncStatus to account for it.
 *
 * This is a copy-paste of pqAppendCmdQueueEntry from fe-exec.c
 *
 * The query itself must already have been put in the output buffer by the
 * caller.
 */
__attribute__((unused)) static void pqAppendCmdQueueEntry(
    PGconn* conn, PGcmdQueueEntry* entry) {
  Assert(entry->next == NULL);

  if (conn->cmd_queue_head == NULL)
    conn->cmd_queue_head = entry;
  else
    conn->cmd_queue_tail->next = entry;

  conn->cmd_queue_tail = entry;

  switch (conn->pipelineStatus) {
    case PQ_PIPELINE_OFF:
    case PQ_PIPELINE_ON:

      /*
       * When not in pipeline aborted state, if there's a result ready
       * to be consumed, let it be so (that is, don't change away from
       * READY or READY_MORE); otherwise set us busy to wait for
       * something to arrive from the server.
       */
      if (conn->asyncStatus == PGASYNC_IDLE) conn->asyncStatus = PGASYNC_BUSY;
      break;

    case PQ_PIPELINE_ABORTED:

      /*
       * In aborted pipeline state, we don't expect anything from the
       * server (since we don't send any queries that are queued).
       * Therefore, if IDLE then do what PQgetResult would do to let
       * itself consume commands from the queue; if we're in any other
       * state, we don't have to do anything.
       */
      if (conn->asyncStatus == PGASYNC_IDLE
#if PG_VERSION_NUM >= 140005
          || conn->asyncStatus == PGASYNC_PIPELINE_IDLE
#endif
      ) {
        resetPQExpBuffer(&conn->errorMessage);
        pqPipelineProcessQueue(conn);
      }
      break;
  }
}

/*
 * pqRecycleCmdQueueEntry
 *		Push a command queue entry onto the freelist.
 *
 * This is a copy-paste of pqRecycleCmdQueueEntry from fe-exec.c
 */
__attribute__((unused)) static void pqRecycleCmdQueueEntry(
    PGconn* conn, PGcmdQueueEntry* entry) {
  if (entry == NULL) return;

  /* recyclable entries should not have a follow-on command */
  Assert(entry->next == NULL);

  if (entry->query) {
    free(entry->query);
    entry->query = NULL;
  }

  entry->next = conn->cmd_queue_recycle;
  conn->cmd_queue_recycle = entry;
}

/*
 * pqCommandQueueAdvance
 *		Remove one query from the command queue, if appropriate.
 *
 * This is a copy-paste of pqCommandQueueAdvance from fe-exec.c
 *
 * If we have received all results corresponding to the head element
 * in the command queue, remove it.
 *
 * In simple query protocol we must not advance the command queue until the
 * ReadyForQuery message has been received.  This is because in simple mode a
 * command can have multiple queries, and we must process result for all of
 * them before moving on to the next command.
 *
 * Another consideration is synchronization during error processing in
 * extended query protocol: we refuse to advance the queue past a SYNC queue
 * element, unless the result we've received is also a SYNC.  In particular
 * this protects us from advancing when an error is received at an
 * inappropriate moment.
 */
static void __attribute__((unused))
pqCommandQueueAdvanceGlue(PGconn *conn, bool isReadyForQuery, bool gotSync)
{
  PGcmdQueueEntry *prevquery;

  if (conn->cmd_queue_head == NULL)
    return;

  /*
   * If processing a query of simple query protocol, we only advance the
   * queue when we receive the ReadyForQuery message for it.
   */
  if (conn->cmd_queue_head->queryclass == PGQUERY_SIMPLE && !isReadyForQuery)
    return;

  /*
   * If we're waiting for a SYNC, don't advance the queue until we get one.
   */
  if (conn->cmd_queue_head->queryclass == PGQUERY_SYNC && !gotSync)
    return;

  /* delink element from queue */
  prevquery = conn->cmd_queue_head;
  conn->cmd_queue_head = conn->cmd_queue_head->next;

  /* If the queue is now empty, reset the tail too */
  if (conn->cmd_queue_head == NULL)
    conn->cmd_queue_tail = NULL;

  /* and make the queue element recyclable */
  prevquery->next = NULL;
  pqRecycleCmdQueueEntry(conn, prevquery);
}

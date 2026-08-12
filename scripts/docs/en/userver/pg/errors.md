# uPg: Postgres errors

Base class for all PostgreSQL errors is Error which is derived from
std::runtime_error. This is done to simplify exception handling.

There are two base types of errors: runtime (RuntimeError) and logic
(LogicError).

**Logic errors** are a consequence of faulty logic within the program such as
violating logical preconditions or invariants and may be preventable by
correcting the program.

**Runtime errors** are due to events beyond the scope of the program, such as
network failure, faulty configuration file, unique index violation etc. A
user can catch such an error and recover from it by reconnecting, providing
a decent default for configuration or modifying the key value.

Both logic and runtime errors can contain a postgres server message
(Message). Those are ServerLogicError and ServerRuntimeError
respectively. These errors occur on the server side and are translated into
exceptions by the driver. Server errors are descendants of either
ServerLogicError or ServerRuntimeError and their hierarchy corresponds to SQL
error classes.

Some server errors, such as IntegrityConstraintViolation, have a more
detailed hierarchy to distinguish errors in catch clauses.

Server errors have the following hierarchy:
  - ServerLogicError
    - SqlStatementNotYetComplete
    - FeatureNotSupported
    - InvalidRoleSpecification
    - CardinalityViolation
    - InvalidObjectName
    - InvalidAuthorizationSpecification
    - SyntaxError
    - AccessRuleViolation
  - ServerRuntimeError
    - TriggeredActionException
    - LocatorException
    - InvalidGrantor
    - DiagnosticsException
    - DataException
    - DuplicatePreparedStatement
    - IntegrityConstraintViolation
      - RestrictViolation
      - NotNullViolation (TODO Make it a logic error)
      - ForeignKeyViolation
      - UniqueViolation
      - CheckViolation
      - ExclusionViolation
      - TriggeredDataChangeViolation
      - WithCheckOptionViolation
    - InvalidCursorState
    - InvalidSqlStatementName
    - InvalidTransactionState
    - DependentPrivilegeDescriptorsStillExist
    - InvalidTransactionTermination
    - ExternalRoutineException
    - ExternalRoutineInvocationException
    - SavepointException
    - SqlRoutineException
    - TransactionRollback
    - InsufficientResources
    - ProgramLimitExceeded
    - ObjectNotInPrerequisiteState
    - OperatorIntervention
      - QueryCancelled
      - AdminShutdown
      - CrashShutdown
      - CannotConnectNow
      - DatabaseDropped
    - SystemError
    - SnapshotFailure
    - ConfigurationFileError
    - FdwError
    - PlPgSqlError
    - InternalServerError

Besides server errors there are exceptions thrown by the driver itself,
those are:
  - LogicError
    - ResultSetError
      - FieldIndexOutOfBounds
      - FieldNameDoesntExist
      - FieldTupleMismatch
      - FieldValueIsNull
      - InvalidBinaryBuffer
      - InvalidInputBufferSize
      - InvalidParserCategory
      - InvalidTupleSizeRequested
      - NarrowingOverflow
      - NonSingleColumnResultSet
      - NonSingleRowResultSet
      - NoBinaryParser
      - RowIndexOutOfBounds
      - TypeCannotBeNull
      - UnknownBufferCategory
    - UserTypeError
      - CompositeSizeMismatch
      - CompositeMemberTypeMismatch
    - ArrayError
      - DimensionMismatch
      - InvalidDimensions
    - NumericError
      - NumericOverflow
      - ValueIsNaN
      - InvalidRepresentation
    - InvalidInputFormat
    - EnumerationError
      - InvalidEnumerationLiteral
      - InvalidEnumerationValue
    - TransactionError
      - AlreadyInTransaction
      - NotInTransaction
    - UnsupportedInterval
    - BoundedRangeError
    - BitStringError
      - BitStringOverflow
      - InvalidBitStringRepresentation
  - RuntimeError
    - ConnectionError
      - ClusterUnavailable
      - CommandError
      - ConnectionFailed
      - ServerConnectionError (contains a message from server)
      - ConnectionTimeoutError
    - ConnectionBusy
    - ConnectionInterrupted
    - PoolError
    - ClusterError
    - InvalidConfig
    - InvalidDSN


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/user_row_types.md | @ref scripts/docs/en/userver/pg/topology.md ⇨
@htmlonly </div> @endhtmlonly

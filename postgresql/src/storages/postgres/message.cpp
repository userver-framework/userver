#include <userver/storages/postgres/message.hpp>

#include <unordered_map>

#include <storages/postgres/detail/result_wrapper.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {

constexpr USERVER_NAMESPACE::utils::TrivialBiMap kStringToSeverity =
    [](auto selector) {
      return selector()
          .Case("LOG", Message::Severity::kLog)
          .Case("DEBUG", Message::Severity::kDebug)
          .Case("INFO", Message::Severity::kInfo)
          .Case("NOTICE", Message::Severity::kNotice)
          .Case("WARNING", Message::Severity::kWarning)
          .Case("ERROR", Message::Severity::kError)
          .Case("FATAL", Message::Severity::kFatal)
          .Case("PANIC", Message::Severity::kPanic);
    };

}  // namespace

Message::Message(detail::ResultWrapperPtr res) : res_{res} {}

std::string Message::GetMessage() const { return res_->GetErrorMessage(); }

std::string Message::GetPrimary() const {
  return res_->GetPrimaryErrorMessage();
}

std::string Message::GetDetail() const { return res_->GetDetailErrorMessage(); }

Message::Severity Message::GetSeverity() const {
  return res_->GetMessageSeverity();
}

std::string Message::GetSeverityString() const {
  return res_->GetMessageSeverityString();
}

SqlState Message::GetSqlState() const { return res_->GetSqlState(); }

std::string Message::GetSchema() const { return res_->GetMessageSchema(); }
std::string Message::GetTable() const { return res_->GetMessageTable(); }
std::string Message::GetColumn() const { return res_->GetMessageColumn(); }
std::string Message::GetDatatype() const { return res_->GetMessageDatatype(); }
std::string Message::GetConstraint() const {
  return res_->GetMessageConstraint();
}

logging::LogExtra Message::GetLogExtra() const {
  return res_->GetMessageLogExtra();
}

void Message::ThrowException() const {
  auto state = GetSqlState();
  if (state == SqlState::kUnknownState) throw LogicError("Empty SQL state");

  auto msg_class = GetSqlStateClass(state);
  switch (msg_class) {
    // Class 03 — SQL Statement Not Yet Complete
    case SqlStateClass::kSqlStatementNotYetComplete:
      throw SqlStatementNotYetComplete{*this};
    // Class 08 - Connection Exception
    case SqlStateClass::kConnectionException:
      throw ServerConnectionError{*this};
    // Class 09 — Triggered Action Exception
    case SqlStateClass::kTriggeredActionException:
      throw TriggeredActionException{*this};
    // Class 0A — Feature Not Supported
    case SqlStateClass::kFeatureNotSupported:
      throw FeatureNotSupported{*this};
    // Class 0B - Invalid Transaction Initiation
    case SqlStateClass::kInvalidTransactionInitiation:
      throw TransactionError{GetMessage()};
    // Class 0F - Locator Exception
    case SqlStateClass::kLocatorException:
      throw LocatorException{*this};
    // Class 0L - Invalid Grantor
    case SqlStateClass::kInvalidGrantor:
      throw InvalidGrantor{*this};
    // Class 0P - Invalid Role Specification
    case SqlStateClass::kInvalidRoleSpecification:
      throw InvalidRoleSpecification{*this};
    // Class 0Z - Diagnostics Exception
    case SqlStateClass::kDiagnosticsException:
      throw DiagnosticsException{*this};
    // Class 20 - Case Not Found
    case SqlStateClass::kCaseNotFound:
      throw SyntaxError{*this};
    // Class 21 - Cardinality Violation
    case SqlStateClass::kCardinalityViolation:
      throw CardinalityViolation{*this};
    // Class 22 - Data Exception
    case SqlStateClass::kDataException:
      throw DataException{*this};
    // Class 23 - Integrity Constraint Violation
    case SqlStateClass::kIntegrityConstraintViolation:
      // Be very specific here
      switch (state) {
        case SqlState::kRestrictViolation:
          throw RestrictViolation{*this};
        case SqlState::kNotNullViolation:
          throw NotNullViolation{*this};
        case SqlState::kForeignKeyViolation:
          throw ForeignKeyViolation{*this};
        case SqlState::kUniqueViolation:
          throw UniqueViolation{*this};
        case SqlState::kCheckViolation:
          throw CheckViolation{*this};
        case SqlState::kExclusionViolation:
          throw ExclusionViolation{*this};
        default:
          throw IntegrityConstraintViolation{*this};
      }
      break;
    // Class 24 - Invalid Cursor State
    case SqlStateClass::kInvalidCursorState:
      throw InvalidCursorState{*this};
    // Class 25 - Invalid Transaction State
    case SqlStateClass::kInvalidTransactionState:
      throw InvalidTransactionState{*this};

    // Class 26 - Invalid SQL Statement Name
    case SqlStateClass::kInvalidSqlStatementName:
      throw InvalidSqlStatementName{*this};
      // Class 34 - Invalid Cursor Name
      // Class 3D - Invalid Catalogue Name
      // Class 3F - Invalid Schema Name
    case SqlStateClass::kInvalidCursorName:
    case SqlStateClass::kInvalidCatalogName:
    case SqlStateClass::kInvalidSchemaName:
      throw InvalidObjectName{*this};

    // Class 27 - Triggered Data Change Violation
    case SqlStateClass::kTriggeredDataChangeViolation:
      throw TriggeredDataChangeViolation{*this};
    // Class 28 - Invalid Authorisation Specification
    case SqlStateClass::kInvalidAuthorizationSpecification:
      throw InvalidAuthorizationSpecification{*this};
    // Class 2B - Dependent Privilege Descriptors Still Exist
    case SqlStateClass::kDependentPrivilegeDescriptorsStillExist:
      throw DependentPrivilegeDescriptorsStillExist{*this};
    // Class 2D - Invalid Transaction Termination
    case SqlStateClass::kInvalidTransactionTermination:
      throw InvalidTransactionTermination{*this};
    // Class 2F - SQL Routine Exception
    case SqlStateClass::kSqlRoutineException:
      throw SqlRoutineException{*this};
    // Class 38 - External Routine Exception
    case SqlStateClass::kExternalRoutineException:
      throw ExternalRoutineException{*this};
    // Class 39 - External Routine Invocation Exception
    case SqlStateClass::kExternalRoutineInvocationException:
      throw ExternalRoutineInvocationException{*this};
    // Class 3B - Savepoint Exception
    case SqlStateClass::kSavepointException:
      throw SavepointException{*this};
    // Class 40 - Transaction Rollback
    case SqlStateClass::kTransactionRollback:
      throw TransactionRollback{*this};
    // Class 42 - Syntax Error Or Access Rule Violation
    case SqlStateClass::kSyntaxErrorOrAccessRuleViolation:
      switch (state) {
        case SqlState::kSyntaxError:
          throw SyntaxError{*this};
        case SqlState::kDuplicatePreparedStatement:
          throw DuplicatePreparedStatement(*this);
        default:
          throw AccessRuleViolation{*this};
      }
    // Class 44 - WITH CHECK OPTION Violation
    case SqlStateClass::kWithCheckOptionViolation:
      throw WithCheckOptionViolation{*this};
    // Class 53 - Insufficient Resources
    case SqlStateClass::kInsufficientResources:
      throw InsufficientResources{*this};
    // Class 54 - Program Limit Exceeded
    case SqlStateClass::kProgramLimitExceeded:
      throw ProgramLimitExceeded{*this};
    // Class 55 - Object Not In Prerequisite State
    case SqlStateClass::kObjectNotInPrerequisiteState:
      throw ObjectNotInPrerequisiteState{*this};
    // Class 57 - Operator Intervention
    case SqlStateClass::kOperatorIntervention:
      // Be more specific here
      switch (state) {
        case SqlState::kQueryCancelled:
          throw QueryCancelled{*this};
        case SqlState::kAdminShutdown:
          throw AdminShutdown{*this};
        case SqlState::kCrashShutdown:
          throw CrashShutdown{*this};
        case SqlState::kCannotConnectNow:
          throw CannotConnectNow{*this};
        case SqlState::kDatabaseDropped:
          throw DatabaseDropped{*this};
        default:
          throw OperatorIntervention{*this};
      }
    // Class 58 - System Error (errors external to PostgreSQL itself)
    case SqlStateClass::kSystemError:
      throw SystemError{*this};
    // Class 72 — Snapshot Failure
    case SqlStateClass::kSnapshotTooOld:
      throw SnapshotFailure{*this};
    // Class F0 — Configuration File Error
    case SqlStateClass::kConfigFileError:
      throw ConfigurationFileError{*this};
    // Class HV — Foreign Data Wrapper Error (SQL/MED)
    case SqlStateClass::kFdwError:
      throw FdwError{*this};
    // Class P0 — PL/pgSQL Error
    case SqlStateClass::kPlpgsqlError:
      throw PlPgSqlError{*this};
    // Class XX — Internal Error
    case SqlStateClass::kInternalError:
      throw InternalServerError{*this};
    default:
      throw ServerLogicError{*this};
  }
}

Message::Severity Message::SeverityFromString(std::string_view str) {
  auto f = kStringToSeverity.TryFind(str);
  if (f) {
    return *f;
  }

  LOG_WARNING() << "Unknown severity string '" << str << '\'';
  return Message::Severity::kLog;
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END

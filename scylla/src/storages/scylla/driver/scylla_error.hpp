#pragma once

#include <string>
#include <string_view>

#include <cassandra.h>

#include <userver/logging/log.hpp>
#include <userver/storages/scylla/exception.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

[[noreturn]] inline void ThrowScyllaError(CassError rc, std::string_view error_message, std::string prefix) {
    if (!prefix.empty()) {
        prefix += ": ";
    }

    const auto source = rc & 0xFF000000u;

    if (source == CASS_ERROR_SOURCE_LIB) {
        switch (rc) {
            case CASS_ERROR_LIB_NO_HOSTS_AVAILABLE:
            case CASS_ERROR_LIB_UNABLE_TO_CONNECT:
            case CASS_ERROR_LIB_UNABLE_TO_CLOSE:
                throw NetworkException() << prefix << error_message;

            case CASS_ERROR_LIB_REQUEST_TIMED_OUT:
                throw TimeoutException(
                    static_cast<int>(rc),
                    utils::StrCat(prefix, error_message),
                    std::chrono::milliseconds{0}
                );

            case CASS_ERROR_LIB_INVALID_STATE:
            case CASS_ERROR_LIB_INVALID_DATA:
            case CASS_ERROR_LIB_INVALID_ERROR_RESULT_TYPE:
            case CASS_ERROR_LIB_INVALID_STATEMENT_TYPE:
                throw InvalidQueryArgumentException() << prefix << error_message;

            default:
                break;
        }
    } else if (source == CASS_ERROR_SOURCE_SERVER) {
        switch (rc) {
            case CASS_ERROR_SERVER_UNAVAILABLE:
            case CASS_ERROR_SERVER_OVERLOADED:
            case CASS_ERROR_SERVER_IS_BOOTSTRAPPING:
                throw ClusterUnavailableException() << prefix << error_message;

            case CASS_ERROR_SERVER_BAD_CREDENTIALS:
                throw AuthenticationException() << prefix << error_message;

            case CASS_ERROR_SERVER_READ_TIMEOUT:
            case CASS_ERROR_SERVER_WRITE_TIMEOUT:
                throw TimeoutException(
                    static_cast<int>(rc),
                    utils::StrCat(prefix, error_message),
                    std::chrono::milliseconds{0}
                );

            case CASS_ERROR_SERVER_READ_FAILURE:
            case CASS_ERROR_SERVER_WRITE_FAILURE:
            case CASS_ERROR_SERVER_FUNCTION_FAILURE:
            case CASS_ERROR_SERVER_ALREADY_EXISTS:
                throw ServerException(static_cast<int>(rc), utils::StrCat(prefix, error_message));

            case CASS_ERROR_SERVER_INVALID_QUERY:
            case CASS_ERROR_SERVER_SYNTAX_ERROR:
            case CASS_ERROR_SERVER_UNPREPARED:
                throw InvalidQueryArgumentException() << prefix << error_message;

            default:
                throw ServerException(static_cast<int>(rc), utils::StrCat(prefix, error_message));
        }
    }

    throw ScyllaException() << prefix << error_message;
}

inline void CheckFuture(CassFuture* future, std::string_view action) {
    CassError rc = cass_future_error_code(future);
    if (rc == CASS_OK) {
        return;
    }

    const char* message = nullptr;
    size_t message_length = 0;
    cass_future_error_message(future, &message, &message_length);
    std::string_view error_message(message, message_length);

    LOG_ERROR() << "Scylla " << action << " failed: " << error_message;

    ThrowScyllaError(rc, error_message, utils::StrCat("Scylla ", action, " failed"));
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END

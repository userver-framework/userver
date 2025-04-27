#pragma once

#include <string>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace settings {
struct ODBCSettings {
    std::string DSN;
};
}  // namespace settings

class Connection final {
public:
    Connection(const settings::ODBCSettings& settings);

    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

private:
    SQLHENV env_;
    SQLHANDLE handle_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END

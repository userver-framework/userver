#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::secdist {

struct OdbcConnectionInfo {
    std::string dsn;
};

class OdbcSettings {
public:
    explicit OdbcSettings(const formats::json::Value& doc);

    std::vector<OdbcConnectionInfo> GetConnectionInfos(const std::string& dbalias) const;

private:
    std::unordered_map<std::string, std::vector<OdbcConnectionInfo>> databases_;
};

}  // namespace storages::odbc::secdist

USERVER_NAMESPACE_END

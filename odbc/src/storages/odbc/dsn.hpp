#pragma once

#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

using Dsn = USERVER_NAMESPACE::utils::NonLoggable<class DsnTag, std::string>;

struct DsnOptions {
    std::string driver;
    std::string server;
    std::string port;
    std::string database;
    std::string uid;
};

DsnOptions ParseDsn(const Dsn& dsn);

std::string GetHostPort(const Dsn& dsn);

std::string DsnCutPassword(const Dsn& dsn);

std::string DsnMaskPassword(const Dsn& dsn);

bool IsIpAddress(std::string_view host);

Dsn ResolveDsnHost(const Dsn& dsn, clients::dns::Resolver& resolver, engine::Deadline deadline);

}  // namespace storages::odbc

USERVER_NAMESPACE_END

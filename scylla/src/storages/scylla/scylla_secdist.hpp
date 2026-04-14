#pragma once

#include <optional>
#include <string>

#include <userver/storages/scylla/session_config.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::secdist {

std::string GetSecdistHosts(const storages::secdist::Secdist& secdist, const std::string& dbalias);

std::string GetSecdistHosts(const storages::secdist::SecdistConfig& secdist, const std::string& dbalias);

std::optional<SslSecrets> GetSecdistSsl(const storages::secdist::Secdist& secdist, const std::string& dbalias);

std::optional<SslSecrets> GetSecdistSsl(const storages::secdist::SecdistConfig& secdist, const std::string& dbalias);

}

USERVER_NAMESPACE_END

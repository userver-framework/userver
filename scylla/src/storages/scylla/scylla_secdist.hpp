#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <userver/storages/scylla/session_config.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::secdist {

std::string GetSecdistHosts(const storages::secdist::Secdist& secdist, std::string_view dbalias);

std::string GetSecdistHosts(const storages::secdist::SecdistConfig& secdist, std::string_view dbalias);

std::optional<SslSecrets> GetSecdistSsl(const storages::secdist::Secdist& secdist, std::string_view dbalias);

std::optional<SslSecrets> GetSecdistSsl(const storages::secdist::SecdistConfig& secdist, std::string_view dbalias);

}  // namespace storages::scylla::secdist

USERVER_NAMESPACE_END

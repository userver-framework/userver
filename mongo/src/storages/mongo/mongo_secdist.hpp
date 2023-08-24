#pragma once

#include <string>

#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::secdist {

std::string GetSecdistConnectionString(
    const storages::secdist::Secdist& secdist, const std::string& dbalias);

}  // namespace storages::mongo::secdist

USERVER_NAMESPACE_END

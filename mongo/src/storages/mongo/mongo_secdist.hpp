#pragma once

#include <string>

#include <userver/storages/secdist/secdist.hpp>

namespace storages::mongo::secdist {

std::string GetSecdistConnectionString(
    const storages::secdist::Secdist& secdist, const std::string& dbalias);

}  // namespace storages::mongo::secdist

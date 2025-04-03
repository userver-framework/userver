#pragma once

#include <boost/uuid/uuid.hpp>

#include <userver/chaotic/convert/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

boost::uuids::uuid Convert(const std::string& str, USERVER_NAMESPACE::chaotic::convert::To<boost::uuids::uuid>);

std::string Convert(const boost::uuids::uuid& uuid, USERVER_NAMESPACE::chaotic::convert::To<std::string>);

}  // namespace chaotic::convert

USERVER_NAMESPACE_END

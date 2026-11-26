#include <handlers/simple/sharedtwo/handler.hpp>

namespace handlers::simple::sharedtwo {

Response View::Handle(Request&&, Deps&&, USERVER_NAMESPACE::server::request::RequestContext&) { return {}; }

}  // namespace handlers::simple::sharedtwo

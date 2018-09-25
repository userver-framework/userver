#pragma once

#include <curl-ev/form.hpp>

namespace clients {
namespace http {

class Form : public curl::form {
  using curl::form::form;
};

}  // namespace http
}  // namespace clients

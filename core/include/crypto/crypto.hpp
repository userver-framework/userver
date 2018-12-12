#pragma once

#include <string>

namespace crypto {

std::string Sha256(const std::string& data);
std::string Sha512(const std::string& data);

bool AreStringsEqualConstTime(const std::string& str1, const std::string& str2);

}  // namespace crypto

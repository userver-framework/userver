#include <blocking/conductor/read_groups.hpp>

#include <boost/algorithm/string/classification.hpp>  // is_any_of
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>

namespace blocking::conductor {

constexpr char hostinfo_path[] = "/etc/conductor-hostinfo/hostinfo";

std::vector<std::string> ReadGroups() {
  std::vector<std::string> split_result;
  std::ifstream ifs(hostinfo_path);
  if (!ifs) {
    throw std::runtime_error("Failed to open file '" +
                             std::string{hostinfo_path} +
                             "' for reading the cgroups.");
  }
  ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                 std::ifstream::eofbit);
  std::string line;

  try {
    while (std::getline(ifs, line)) {
      boost::algorithm::erase_all(line, " ");
      if (line.find("groups=") != 0) {
        continue;
      }

      if (const auto pos = line.find("'"); pos != std::string::npos) {
        line = line.substr(pos + 1, line.find_last_of("'") - pos - 1);
      } else {
        line = line.substr(sizeof("groups"));
      }

      boost::algorithm::split(split_result, line,
                              boost::algorithm::is_any_of(","),
                              boost::algorithm::token_compress_on);
      break;
    }
  } catch (const std::exception& e) {
    throw std::runtime_error(
        "Error during search and parse of 'groups' record in '" +
        std::string{hostinfo_path} + "': " + e.what());
  }

  return split_result;
}

}  // namespace blocking::conductor

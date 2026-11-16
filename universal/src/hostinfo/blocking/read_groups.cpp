#include <userver/hostinfo/blocking/read_groups.hpp>

#include <fstream>
#include <stdexcept>

#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace hostinfo::blocking {
namespace {

constexpr auto kHostinfoPath = "/etc/conductor-hostinfo/hostinfo";

}  // namespace

std::vector<std::string> ReadConductorGroups() {
    std::vector<std::string> split_result;
    std::ifstream ifs(kHostinfoPath);
    if (!ifs) {
        throw std::runtime_error("Failed to open file '" + std::string{kHostinfoPath} + "' for reading the cgroups.");
    }
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
    std::string line;

    try {
        while (std::getline(ifs, line)) {
            std::erase(line, ' ');
            if (line.find("groups=") != 0) {
                continue;
            }

            if (const auto pos = line.find('\''); pos != std::string::npos) {
                line = line.substr(pos + 1, line.find_last_of('\'') - pos - 1);
            } else {
                line = line.substr(sizeof("groups"));
            }

            split_result = utils::text::Split(line, ",");
            break;
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(
            "Error during search and parse of 'groups' record in '" + std::string{kHostinfoPath} + "': " + e.what()
        );
    }

    return split_result;
}

}  // namespace hostinfo::blocking

USERVER_NAMESPACE_END

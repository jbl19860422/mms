#include <sstream>

#include "base/utils/utils.h"
#include "rtcp.h"

using namespace mms;
std::string RtcpAttr::prefix = "a=rtcp:";
bool RtcpAttr::parse(const std::string & line) {
    std::string::size_type end_pos = line.rfind("\r");
    if (end_pos == std::string::npos) {
        end_pos = line.size() - 1;
    }
    std::string valid_string = line.substr(prefix.size(), end_pos);

    std::vector<std::string> vs;
    vs = Utils::split(valid_string, " ");
    if (vs.size() < 1) {
        return false;
    }

    port = std::atoi(vs[0].c_str());
    if (vs.size() >= 2) {
        nettype = vs[1];
    }

    if (vs.size() >= 3) {
        addrtype = vs[2];
    }
    
    if (vs.size() >= 4) {
        connection_address = vs[3];
    }
    return true;
}

std::string RtcpAttr::toString() const {
    std::ostringstream oss;
    oss << prefix << nettype << " " << addrtype << " " << connection_address;
    oss << std::endl;
    return oss.str();
}
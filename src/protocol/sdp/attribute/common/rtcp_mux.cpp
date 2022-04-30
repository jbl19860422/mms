#include <sstream>
#include "rtcp_mux.h"
using namespace mms;
std::string RtcpMux::prefix = "a=rtcp-mux";
bool RtcpMux::parse(const std::string &line)
{
    std::string::size_type end_pos = line.rfind("\r");
    if (end_pos == std::string::npos)
    {
        end_pos = line.size() - 1;
    }

    return true;
}

std::string RtcpMux::toString() const
{
    std::ostringstream oss;
    oss << prefix << std::endl;
    return oss.str();
}
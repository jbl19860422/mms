#include <sstream>
#include "recvonly.hpp"
using namespace mms;
std::string RecvOnlyAttr::prefix = "a=recvonly";
bool RecvOnlyAttr::parse(const std::string &line)
{
    std::string::size_type end_pos = line.rfind("\r");
    if (end_pos == std::string::npos)
    {
        end_pos = line.size() - 1;
    }

    return true;
}

std::string RecvOnlyAttr::toString() const
{
    std::ostringstream oss;
    oss << prefix << std::endl;
    return oss.str();
}
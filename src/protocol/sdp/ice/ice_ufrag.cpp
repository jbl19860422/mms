#include <sstream>

#include "ice_ufrag.h"
#include "base/utils/utils.h"
using namespace mms;
std::string IceUfrag::prefix = "a=ice-ufrag:";
bool IceUfrag::parse(const std::string & line) {
    std::string::size_type end_pos = line.rfind("\r");
    if (end_pos == std::string::npos) {
        end_pos = line.size() - 1;
    }
    ufrag = line.substr(prefix.size(), end_pos);
    return true;
}

std::string IceUfrag::toString() const {
    std::ostringstream oss;
    oss << prefix << ufrag << std::endl;
    return oss.str();
}
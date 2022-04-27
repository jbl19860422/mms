#pragma once
#include <string>
// 5.3.  Session Name ("s=")
// s=<session name>
namespace mms {
struct SessionName {
public:
    static std::string prefix;
    bool parse(const std::string & line);
public:
    std::string raw_string;
    std::string valid_string;
    //    The "s=" field is the textual session name.  There MUST be one and
    //    only one "s=" field per session description.  The "s=" field MUST NOT
    //    be empty and SHOULD contain ISO 10646 characters (but see also the
    //    "a=charset" attribute).  If a session has no meaningful name, the
    //    value "s= " SHOULD be used (i.e., a single space as the session
    //    name).
    std::string session_name;
};
};
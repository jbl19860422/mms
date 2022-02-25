#pragma once
#include <string_view>

namespace mms {
// 5.5.  URI ("u=")
//       u=<uri>
//    A URI is a Uniform Resource Identifier as used by WWW clients [7].
//    The URI should be a pointer to additional information about the
//    session.  This field is OPTIONAL, but if it is present it MUST be
//    specified before the first media field.  No more than one URI field
//    is allowed per session description.
class Uri {

public:
    std::string_view raw_string;
    std::string_view uri;
};
};
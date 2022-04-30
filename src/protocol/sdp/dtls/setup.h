#pragma once
// https://tools.ietf.org/html/rfc4145#section-4
#include <string>
namespace mms {
struct SetupAttr {
public:
    static std::string prefix;
    bool parse(const std::string & line);
    std::string toString() const;
    const std::string & getRole() const {
        return role;
    }

    void setRole(const std::string &val) {
        role = val;
    }
public:
    std::string role;
};
};
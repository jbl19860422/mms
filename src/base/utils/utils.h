#pragma once
#include <vector>
#include <string>
#include <string_view>
namespace mms {
class Utils {
public:
    static std::vector<std::string_view> split(std::string_view strv, std::string_view delims = " ");
    static std::vector<std::string> split(const std::string &strv, const std::string &delims);
    static bool startWith(const std::string_view &str, const std::string_view &prefix);
    static bool endWith(const std::string_view &str, const std::string_view &suffix);
    static uint64_t rand64();
    static std::string randStr(size_t len);
    static uint32_t getCRC32(uint8_t* buf, size_t len);
    static std::string calcHmacSHA256(const std::string &key, const std::string & msg);
    static std::string calcHmacSHA1(const std::string &key, const std::string & msg);
    static std::string sha256(const std::string & msg);
    static std::string sha1(const std::string & msg);
};
};
#pragma once
#include <string>
#include "openssl/hmac.h"
#include "openssl/sha.h"

#include "stun_define.hpp"

namespace mms
{

    /*
    MESSAGE-INTEGRITY：STUN 消息的 HMAC-SHA1 值，长度 20 字节，用于消息完整性认证。详细的计算方式后续进行举例说明。
    text：STUN的包头+属性集合到MESSAGE-INTEGRITY位置，后续的属性不计算在内，但是需要更改stun的包头的length值，此长度需要包含MESSAGE-INTEGRITY的长度，但不包含后面属性的长度。
    属性值：短期认证：HMAC-SHA1计算的password值。计算方法：HMAC_SHA1(PASSWORD)

　　　　　　长期认证：MD5(username ":" realm ":" SASLprep(password))

　　对于短期认证计算网址：https://www.liavaag.org/English/SHA-Generator/HMAC/

    15.4.  MESSAGE-INTEGRITY

       The MESSAGE-INTEGRITY attribute contains an HMAC-SHA1 [RFC2104] of
       the STUN message.  The MESSAGE-INTEGRITY attribute can be present in
       any STUN message type.  Since it uses the SHA1 hash, the HMAC will be
       20 bytes.  The text used as input to HMAC is the STUN message,
       including the header, up to and including the attribute preceding the
       MESSAGE-INTEGRITY attribute.  With the exception of the FINGERPRINT
       attribute, which appears after MESSAGE-INTEGRITY, agents MUST ignore
       all other attributes that follow MESSAGE-INTEGRITY.




    Rosenberg, et al.           Standards Track                    [Page 34]

    RFC 5389                          STUN                      October 2008


       The key for the HMAC depends on whether long-term or short-term
       credentials are in use.  For long-term credentials, the key is 16
       bytes:

                key = MD5(username ":" realm ":" SASLprep(password))

       That is, the 16-byte key is formed by taking the MD5 hash of the
       result of concatenating the following five fields: (1) the username,
       with any quotes and trailing nulls removed, as taken from the
       USERNAME attribute (in which case SASLprep has already been applied);
       (2) a single colon; (3) the realm, with any quotes and trailing nulls
       removed; (4) a single colon; and (5) the password, with any trailing
       nulls removed and after processing using SASLprep.  For example, if
       the username was 'user', the realm was 'realm', and the password was
       'pass', then the 16-byte HMAC key would be the result of performing
       an MD5 hash on the string 'user:realm:pass', the resulting hash being
       0x8493fbc53ba582fb4c044c456bdc40eb.

       For short-term credentials:

                              key = SASLprep(password)

       where MD5 is defined in RFC 1321 [RFC1321] and SASLprep() is defined
       in RFC 4013 [RFC4013].

       The structure of the key when used with long-term credentials
       facilitates deployment in systems that also utilize SIP.  Typically,
       SIP systems utilizing SIP's digest authentication mechanism do not
       actually store the password in the database.  Rather, they store a
       value called H(A1), which is equal to the key defined above.

       Based on the rules above, the hash used to construct MESSAGE-
       INTEGRITY includes the length field from the STUN message header.
       Prior to performing the hash, the MESSAGE-INTEGRITY attribute MUST be
       inserted into the message (with dummy content).  The length MUST then
       be set to point to the length of the message up to, and including,
       the MESSAGE-INTEGRITY attribute itself, but excluding any attributes
       after it.  Once the computation is performed, the value of the
       MESSAGE-INTEGRITY attribute can be filled in, and the value of the
       length in the STUN header can be set to its correct value -- the
       length of the entire message.  Similarly, when validating the
       MESSAGE-INTEGRITY, the length field should be adjusted to point to
       the end of the MESSAGE-INTEGRITY attribute prior to calculating the
       HMAC.  Such adjustment is necessary when attributes, such as
       FINGERPRINT, appear after MESSAGE-INTEGRITY.
    */
    struct StunMessageIntegrityAttr : public StunMsgAttr
    {
        StunMessageIntegrityAttr(uint8_t *data, size_t len, bool has_finger_print, const std::string & pwd);
        StunMessageIntegrityAttr() = default;
        size_t size();

        int32_t encode(uint8_t *data, size_t len);

        int32_t decode(uint8_t *data, size_t len);

        std::string hmac_sha1; // 20字节
    };

};
#pragma once
#include <boost/array.hpp>

#include "base/network/tcp_socket.hpp"

namespace mms {
class RtmpHandshake {
public:
    RtmpHandshake(RtmpConn *conn):conn_(conn) {
    }

    virtual ~RtmpHandshake() {
    }

    bool handshake() {
        boost::array<char, 1537> c0c1;
        if(!conn_->recv(c0c1.data(), 1537)) {
            return false;
        }

        boost::array<char, 3073> s0s1s2;
        _genS0S1S2(c0c1.data(), s0s1s2.data());
        // send s0, s1, s2
        if(!conn_->send(s0s1s2.data(), 3073)) {
            return false;
        }

        boost::array<char, 1536> c2;
        if(!conn_->recv(c2.data(), 1536)) {
            return false;
        }
        return true;
    }

    void _genS0S1S2(char *c0c1, char *s0s1s2) {
        //s0
        s0s1s2[0] = '\x03';
        //s1
        memset(s0s1s2 + 1, 0, 8);
        //s2
        int32_t t = ntohl(*(int32_t*)(c0c1 + 1));
        *(int32_t*)(s0s1s2 + 1537) = htonl(t);
        *(int32_t*)(s0s1s2 + 1541) = htonl(time(NULL));
        memcpy(s0s1s2 + 1545, c0c1 + 9, 1528);
    }
private:
    RtmpConn *conn_;
};
};
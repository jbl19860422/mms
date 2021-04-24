#pragma once
#include <memory>

#include "server/tcp/tcp_server.hpp"
#include "rtmp_conn.hpp"
#include "base/thread/thread_pool.hpp"

namespace mms {
class RtmpServer : public TcpServer<RtmpConn>, ServerConnHandler<RtmpConn> {
public:
    RtmpServer(ThreadWorker *w):TcpServer(w) {
        setConnHandler(this);
    }
    
    bool start() {
        if (0 == startListen(1935)) {
            return true;
        }
        return false;
    }

    void stop() {
        stopListen();
    }
private:
    void onConnOpen(RtmpConn *conn);
    void onConnClosed(RtmpConn *conn);
};
};
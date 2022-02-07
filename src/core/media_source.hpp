#pragma once
#include <memory>
#include <vector>

#include "server/rtmp/rtmp_conn/rtmp_conn.hpp"
#include "server/rtmp/rtmp_protocol/rtmp_handshake.hpp"
#include "server/rtmp/rtmp_protocol/rtmp_chunk_protocol.hpp"

namespace mms {
class MediaSource : public MediaStream {
public:
    MediaSource(ThreadWorker *worker) : MediaStream(worker){
        
    }

    virtual ~MediaSource() {

    }
};

};
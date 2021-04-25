#pragma once
#include "./amf0/amf0_object.hpp"

namespace mms {
class RtmpMessage {
public:
    RtmpMessage(int32_t payload_size) {
        payload_ = new char[payload_size];
    }

    virtual ~RtmpMessage() {
        if(payload_) {
            delete payload_;
            payload_ = nullptr;
        }
    }

    std::vector<boost::shared_ptr<RtmpChunk>> toChunks(uint8_t chunk_id, uint32_t chunk_size) {
        
    }
public:
    char *payload_ = nullptr;
    int32_t curr_size_ = 0;
    int32_t timestamp_;
    uint8_t message_type_id_;
    int32_t message_stream_id_;
};

class ChunkMessageHeader {
public:
    int32_t timestamp_;
    int32_t message_length_;
    uint8_t message_type_id_;
    int32_t message_stream_id_;
};

class RtmpChunk {
public:
    RtmpChunk& operator=(RtmpChunk & c) {
        memcpy(&this->chunk_message_header_, &c.chunk_message_header_, sizeof(ChunkMessageHeader));
        this->rtmp_message_ = c.rtmp_message_;
        c.rtmp_message_ = nullptr;
        return *this;
    }
public:
    ChunkMessageHeader chunk_message_header_;
    RtmpMessage *rtmp_message_ = nullptr;
};
};
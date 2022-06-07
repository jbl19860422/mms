#include <string.h>
#include <iostream>
#include <arpa/inet.h>

#include "client_hello.h"
#include "dtls_handshake.h"
using namespace mms;

int32_t HandShake::decode(uint8_t *data, size_t len)
{
    uint8_t *data_start = data;
    if (len < 1)
    {
        return -1;
    }
    msg_type = (HandshakeType)data[0];
    data++;
    len--;

    if (len < 3)
    {
        return -2;
    }
    uint8_t *p = (uint8_t *)&length;
    p[0] = data[2];
    p[1] = data[1];
    p[2] = data[0];
    data += 3;
    len -= 3;

    if (len < 2) 
    {
        return -3;
    }
    message_seq = ntohs(*(uint16_t*)data);
    data += 2;
    len -= 2;

    if (len < 3) 
    {
        return -4;
    }
    p = (uint8_t *)&fragment_offset;
    p[0] = data[2];
    p[1] = data[1];
    p[2] = data[0];
    data += 3;
    len -= 3;

    if (len < 3) 
    {
        return -5;
    }
    p = (uint8_t *)&fragment_length;
    p[0] = data[2];
    p[1] = data[1];
    p[2] = data[0];
    data += 3;
    len -= 3;
    std::cout << "fragment_length=" << fragment_length << std::endl;


    if (msg_type == client_hello)
    {
        msg = std::unique_ptr<HandShakeMsg>(new ClientHello);
    }

    if (!msg)
    {
        return -6;
    }

    int32_t c = msg->decode(data, len);
    if (c < 0)
    {
        std::cout << "client decode failed, code:" << c << std::endl;
        return -7;
    }
    data += c;
    len -= c;

    return data - data_start;
}

int32_t HandShake::encode(uint8_t *data, size_t len)
{
    uint8_t *data_start = data;
    if (len < 1) 
    {
        return -1;
    }
    data[0] = msg_type;
    data++;
    len--;

    uint8_t *plen = data;
    data += 3;
    len -= 3;
    uint32_t content_len = 0;
    if (msg)
    {
        int32_t c = msg->encode(data, len);
        if (c < 0)
        {
            return -2;
        }
        data += c;
        len -= c;
        content_len += c;
    }

    uint8_t * p = (uint8_t*)&content_len;
    plen[0] = p[2];
    plen[1] = p[1];
    plen[2] = p[0];
    return data - data_start;
}

uint32_t HandShake::size()
{
    uint32_t s = 4;
    s += msg->size();
    return s;
}
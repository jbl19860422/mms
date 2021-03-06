#include "stun_server.hpp"
#include "protocol/stun_binding_error_response_msg.hpp"
#include "protocol/stun_binding_response_msg.hpp"
#include "protocol/stun_mapped_address_attr.h"

using namespace mms;

void StunServer::onUdpSocketRecv(UdpSocket *sock, std::unique_ptr<uint8_t[]> data, size_t len, boost::asio::ip::udp::endpoint &remote_ep)
{
    auto worker = thread_pool_inst::get_mutable_instance().getWorker(-1);
    boost::asio::spawn(worker->getIOContext(), [this, sock, recv_data = std::move(data), len, remote_ep](boost::asio::yield_context yield) {
        StunMsg stun_msg;
        int32_t ret = stun_msg.decode(recv_data.get(), len);
        if (0 != ret) {
            std::cout << "decode stun message failed, code:" << ret << ", len:" << len << std::endl;
            return;
        }
        std::cout << "stun_msg.type()=" << (uint32_t)stun_msg.type() << std::endl;
        switch(stun_msg.type()) {
            case STUN_BINDING_REQUEST : {
                std::cout << "process binding resquest" << std::endl;
                processBindMsg(stun_msg, sock, remote_ep, yield);
                break;
            }
        } 
    });
}

void StunServer::processBindMsg(StunMsg &msg, UdpSocket *sock, const boost::asio::ip::udp::endpoint &remote_ep, boost::asio::yield_context &yield)
{
    // if (msg.attrs.size() <= 0) {// no message integrity  //不知道为什么，返回401，仍然不带message integrity
    //     StunBindingErrorResponseMsg resp(msg, 401, "");
    //     auto s = resp.size();

    //     std::unique_ptr<uint8_t[]> data = std::unique_ptr<uint8_t[]>(new uint8_t[s]);
    //     int32_t consumed = resp.encode(data.get(), s);
    //     if (consumed < 0) {
    //         return;
    //     }
    //     if (!sock->sendTo(std::move(data), s, remote_ep, yield)) {//todo log error
    //     }
    //     return;
    // }
    StunBindingResponseMsg binding_resp(msg);
    std::cout << "remote port:" << remote_ep.port() << std::endl;
    auto mapped_addr_attr = std::make_unique<StunMappedAddressAttr>(remote_ep.address().to_v4().to_uint(), remote_ep.port());
    binding_resp.addAttr(std::move(mapped_addr_attr));
    auto size = binding_resp.size();
    std::unique_ptr<uint8_t[]> data = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
    int32_t consumed = binding_resp.encode(data.get(), size);
    if (consumed < 0)
    { // todo:add log
        return;
    }

    if (!sock->sendTo(std::move(data), size, remote_ep, yield))
    { // todo log error
    }
}

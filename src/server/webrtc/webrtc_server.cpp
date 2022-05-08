#include "webrtc_server.hpp"
#include "websocket_conn.hpp"
#include "webrtc_session.hpp"
#include "config/config.h"


#include "server/stun/protocol/stun_define.hpp"
#include "server/stun/protocol/stun_msg.h"

using namespace mms;

bool WebRtcServer::start() {
    bool ret = WebsocketServer::start();
    if (!ret) {
        return false;
    }

    ret = UdpServer::startListen(Config::getInstance().getWebrtcUdpPort());
    if (!ret) {
        return false;
    }   
    return true;
}

void WebRtcServer::onUdpSocketRecv(UdpSocket *sock, std::unique_ptr<uint8_t[]> data, size_t len, boost::asio::ip::udp::endpoint &remote_ep) {
    auto worker = thread_pool_inst::get_mutable_instance().getWorker(-1);
    std::cout << "webrtc server recv len:" << len << std::endl;
    boost::asio::spawn(worker->getIOContext(), [this, sock, recv_data = std::move(data), len, remote_ep](boost::asio::yield_context yield) {
        StunMsg stun_msg;
        int ret = stun_msg.decode(recv_data.get(), len);
        if (0 != ret) {
            std::cout << "webrtc decode stun msg failed. ret:" << ret << std::endl;
            return;
        }

        std::cout << "stun_msg.type()=" << (uint32_t)stun_msg.type() << std::endl;
        switch(stun_msg.type()) {
            case STUN_BINDING_REQUEST : {
                std::cout << "process binding resquest" << std::endl;
                // 校验完整性
                // 校验fingerprint
                // 返回响应
                break;
            }
        }
    });
}

void WebRtcServer::onWebsocketOpen(websocketpp::connection_hdl hdl) {
    try {
        websocketpp::server<websocketpp::config::asio>::connection_ptr conn = get_con_from_hdl(hdl);
        std::shared_ptr<WebSocketConn> ws_conn = std::make_shared<WebSocketConn>(worker_, conn); 
        auto webrtc_session = ws_conn->createSession();
        {
            std::lock_guard<std::mutex> lck(mtx_);
            conn_map_.insert(std::pair(conn, ws_conn));
        }
        {
            std::lock_guard<std::mutex> lck(ufrag_session_map_mtx_);
            ufrag_session_map_.insert(std::pair(webrtc_session->getICEUfrag(), webrtc_session));
        }
        
        conn->set_message_handler(std::bind(&WebRtcSession::onMessage, webrtc_session.get(), this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

        webrtc_session->service();
    } catch (std::exception & e) {
        std::cout << "open websocket failed." << std::endl;
    }
}

void WebRtcServer::onWebsocketClose(websocketpp::connection_hdl hdl) {
    websocketpp::server<websocketpp::config::asio>::connection_ptr conn = get_con_from_hdl(hdl);
    std::shared_ptr<WebSocketConn> ws_conn;
    {
        std::lock_guard<std::mutex> lck(mtx_);
        auto it = conn_map_.find(conn);
        if (it == conn_map_.end()) {
            return;
        }
        ws_conn = it->second;
    }
    
    conn_map_.erase(conn);
}
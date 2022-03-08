#pragma once
#include <unordered_map>
#include <memory>

#include "server/udp/udp_server.hpp"
#include "websocket_server.hpp"
namespace mms {
class WebSocketConn;
class WebRtcServer : public UdpServer, public WebsocketServer {
public:
    WebRtcServer(ThreadWorker *worker) : UdpServer(worker), WebsocketServer(worker) {
        worker_ = worker;
    }

    bool start();
private:
    void onUdpSocketRecv(UdpSocket *sock, std::unique_ptr<uint8_t[]> data, size_t len, boost::asio::ip::udp::endpoint &remote_ep) override;
private:
    virtual void onWebsocketOpen(websocketpp::connection_hdl hdl);
    virtual void onWebsocketClose(websocketpp::connection_hdl hdl);
private:
    ThreadWorker *worker_;
    std::mutex mtx_;
    std::unordered_map<websocketpp::server<websocketpp::config::asio>::connection_ptr, std::shared_ptr<WebSocketConn>> conn_map_;
};
};
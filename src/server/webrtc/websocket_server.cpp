#include "websocket_server.hpp"
using namespace mms;
WebsocketServer::WebsocketServer(ThreadWorker *worker) : worker_(worker) {

}

WebsocketServer::~WebsocketServer() {

}

bool WebsocketServer::start(uint16_t port) {
    if (!worker_) {
        return false;
    }

    worker_->post([port, this]() {
        try {
            // Set logging settings
            set_access_channels(websocketpp::log::alevel::all);
            clear_access_channels(websocketpp::log::alevel::frame_payload);

            // Initialize Asio
            init_asio();
            // Register our message handler
            set_open_handler(std::bind(&WebsocketServer::onWebsocketOpen, this, websocketpp::lib::placeholders::_1));
            set_close_handler(std::bind(&WebsocketServer::onWebsocketClose, this, websocketpp::lib::placeholders::_1));
            set_message_handler(std::bind(&WebsocketServer::onMessage, this, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

            // Listen on port
            listen(port);

            // Start the server accept loop
            start_accept();

            // Start the ASIO io_service run loop
            run();
        } catch (websocketpp::exception const & e) {
            std::cout << e.what() << std::endl;
        } catch (...) {
            std::cout << "other exception" << std::endl;
        }
    });

    return 0;
}

void WebsocketServer::onMessage(websocketpp::server<websocketpp::config::asio>* server, websocketpp::connection_hdl hdl, message_ptr msg) {

}

void WebsocketServer::stop() {

}
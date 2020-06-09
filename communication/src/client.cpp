
#include "client_handler.h"
#include "client.hpp"


void Client::run()
{
    if(!workable) {
        return ;
    }
    string addr = config.get<string>("server.addr");
    int port = config.get<int>("server.port");
    handler = make_shared<ClientHandler>(addr, port, shared_from_this());
    std::thread server_handle_thread(boost::bind(&ClientHandler::start, handler, uid));
    server_handle_thread.join();
}
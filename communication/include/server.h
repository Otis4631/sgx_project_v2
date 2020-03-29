
#pragma once

#include "server_all.h"
typedef shared_ptr<ServerHandler> s_ptr;

class Server
{
public:
    Server(bpt::ptree _conf): config(_conf) {
        string ip = config.get<string>("server.listen_addr");
        int port = config.get<int>("server.listen_port");

        ep = asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port);
        acc = make_shared<asio::ip::tcp::acceptor>(service, ep);
    }
    void run()
    {
        LOG_NOTICE(log) << "Server running ...";
        handle_accept(NULL, b_error_code());
        service.run();
    }
    void handle_accept(s_ptr handler, const b_error_code &err)
    {
        if(err) {
            LOG_ERROR(log) << "Error occurred when handle accept";
        }
        if(handler) {
            handler->start();
            clients.push_back(handler);
        }
        s_ptr new_handler(new ServerHandler());
        acc->async_accept(new_handler->get_sock(), boost::bind(&Server::handle_accept, this, new_handler, _1));
    }

private:
    bpt::ptree config;
    vector<s_ptr> clients;
    // asio::ip::tcp::acceptor acceptor; // (service, ip::tcp::endpoint(ip::tcp::v4(),8001));
    asio::io_service service;
    asio::ip::tcp::endpoint ep;
    shared_ptr<asio::ip::tcp::acceptor> acc;
    src::severity_logger<severity_level> log;
};

#pragma once
#include "all.h"
#include <stdint.h>
#include <string>

using namespace std;
namespace asio = boost::asio;

typedef boost::shared_ptr<asio::ip::tcp::socket> p_socket_t;

class Data {

};

class Handler: public asio::coroutine {
    public:
        Handler(string& _addr, int& _port)
            :  local_sock(service), stopping(false){
                asio::ip::tcp::resolver resolver(service);
                peer_ep = *resolver.resolve(asio::ip::tcp::resolver::query(_addr, to_string(_port)));
            }
        Handler();
        virtual void run() = 0;
        virtual void step() = 0;
        virtual void gen_crypt_package(int version, int cmd, int k_len, int iv_len, int n_len) = 0;
        virtual void on_read() = 0;
        virtual void on_write() = 0;


    protected:
        asio::io_service service;
        asio::ip::tcp::socket local_sock;
        asio::ip::tcp::endpoint peer_ep;

        string addr;
        int port;
        bool stopping;
        asio::streambuf read_buff;
        asio::streambuf write_buff;

        int stage;
};

   

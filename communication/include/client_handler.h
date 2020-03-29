#pragma once
#include "client_all.h"

typedef class ClientHandler self_type;
#define BIND_FN(x)       boost::bind(&self_type::x, shared_from_this())
#define BIND_FN1(x,y)    boost::bind(&self_type::x, shared_from_this(),y)
#define BIND_FN2(x,y,z)  boost::bind(&self_type::x, shared_from_this(),y,z)

class ClientHandler :public Handler, public std::enable_shared_from_this<ClientHandler> {
    public:
        ClientHandler(string& addr, int& port) 
            : Handler(addr, port), stage(STAGE_INIT) {
                LOG_NAME("ClientHandler");
        }
        ClientHandler(){};
        void do_connect(asio::ip::tcp::endpoint& ep);
        void on_connect(const b_error_code & err);
        void on_connect_err(const b_error_code & err);
        
        void do_read();
        void on_read(const b_error_code& ec, size_t size);

        void do_write();
        void on_write(const b_error_code& ec, size_t size);

        void start(string& _uid);

        size_t read_completion(const b_error_code & err, size_t bytes);

        bool stage_init_read();

        ~ClientHandler() {
            if(!stopping)   stop();
        }

    private:
        string uid;
        int stage;
};
#include "all.h"

#include <boost/asio/yield.hpp>
#include <boost/asio/coroutine.hpp>
typedef boost::system::error_code b_error_code;

class ClientHandler :public Handler, public std::enable_shared_from_this<ClientHandler> {
    public:
        ClientHandler(string& addr, int& port) 
            : Handler(addr, port), stage(CLIENT_STAGE_INIT) {
        }
        ClientHandler(){};
        void step(const b_error_code & err = b_error_code(), size_t bytes = 0);
        void do_init(const b_error_code & err = b_error_code(), size_t bytes = 0);
        void gen_init_package(int8_t cmd, string data, int8_t verion = 1);
        void on_read(const b_error_code& ec, size_t size);
        void on_write(const b_error_code& ec, size_t size);
        void init(string& _uid);
    private:
        string uid;
        int stage;


};

// void ClientHandler::step(const b_error_code& err, size_t bytes) {
//     reenter(this) {
//          yield asio::async_write(local_sock, write_buff, BIND_FN2(&ClientHandler::on_write, _1, _2));
//          yield asio::async_read(local_sock, read_buff, "\n",BIND_FN2(&ClientHandler::on_read,_1,_2));
//         // yield service.post(MEM_FN(on_answer_from_server));
//     }
// }
void ClientHandler::do_write() {

}


void ClientHandler::on_read(const b_error_code& ec, size_t size) {

}
void ClientHandler::on_write(const b_error_code& ec, size_t size) {
    
}

void ClientHandler::init(string& _uid) {
    uid = _uid;
    gen_init_package(1, uid);
}

void ClientHandler::do_init(const b_error_code & err, size_t bytes) {
    reenter(this) 
    { 
        for (;;) {
            yield local_sock.async_connect(peer_ep, BIND_FN2(&ClientHandler::do_init,_1,_2));
            yield asio::async_write(local_sock, write_buff, BIND_FN2(&ClientHandler::do_init, _1, _2));
            yield asio::async_read(local_sock, read_buff, asio::transfer_exactly(BUFF_SIZE), BIND_FN2(&ClientHandler::on_read,_1,_2));
        }
    } 
}

void ClientHandler::gen_init_package(int8_t cmd, string data, int8_t verion) {
    write_buff.consume(BUFF_SIZE);
    ostream out(&write_buff);
    out << verion << cmd << data;
    //LOG_DEBUG << out;
}
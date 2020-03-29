

#include "server_all.h"


void ServerHandler::start()
{
    LOG_DEBUG(log) << "Got Connection from" << local_sock.remote_endpoint();
    last_ping = boost::posix_time::microsec_clock::local_time();
    do_read();
    run();
}


void ServerHandler::on_write(const b_error_code &ec, size_t size) {
    if(ec) {do_stop(ec.message()); return ;}
    if(stage == STAGE_INIT) {
        auto cb = write_buff.data();
        std::string cmd( boost::asio::buffers_begin(cb) + 1, boost::asio::buffers_begin(cb) + 2);
        cout << std::hex << cmd;
        if(stoi(cmd) == 0x11) // ok
            stage == STAGE_CRYPT;
        else if(stoi(cmd) == 0x12)
            stage == STAGE_DESTORYED;
    }

    do_read();
}

void ServerHandler::do_read()
{
    if (stopping)
        return;
    if (stage == STAGE_INIT)
    {
        asio::async_read(local_sock, read_buff.prepare(BUFF_SIZE), BIND_FN2(on_read, _1, _2));
    }
}

void ServerHandler::on_read(const b_error_code &ec, size_t size)
{
    if (ec)
    {
        LOG_ERROR(log) << "Error occurred " << ec.message();
        stop();
    }
    read_buff.commit(size);

    if (stage == STAGE_INIT)
    {
        istream in(&read_buff);
        char ver, cmd;
        string _uid;

        in >> ver >> cmd >> _uid;


        if (cmd == 0x01)
        {
            if(!auth_uid(_uid)) {
                gen_init_package(0x12, ""); // uid is incorrect
                LOG_ERROR(log) << ("uid is incorrect");
            }
            gen_init_package(0x11, ""); // uid is ok
        }
        else {
            do_stop("Unknown command type in stage init");
        }
    }
    do_write();
}

void ServerHandler::do_write() {
    if(stopping) return ;
    if(stage == STAGE_INIT) {
        asio::async_write(local_sock, write_buff.data(), BIND_FN2(on_write, _1, _2));
    }
}



size_t ServerHandler::read_completion(const b_error_code &err, size_t bytes)
{
    if (stopping)
        return 0;
    if (err)
    {
        LOG_ERROR(log) << "Error occurred " << err.message();
        stop();
        return 0;
    }
    if (stage == STAGE_INIT)
    {
        if (read_buff.size() < HELLO_HEADER_SIZE)
        {
            LOG_DEBUG(log) << "Read " << read_buff.size() << " more data to read";
            return 1;
        }
    }
    return 0;
}

bool ServerHandler::auth_uid(string& s)
{
    if(s == "lizheng") return true;
    return false;
}



void ServerHandler::do_stop(string e) {
    LOG_ERROR(log) << e;
    stop();
}
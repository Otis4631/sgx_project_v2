#include "client_all.h"

#include <cmath>


void ClientHandler::do_connect(asio::ip::tcp::endpoint &ep)
{
    if (stopping)
        return;
    if (stage == STAGE_INIT)
    {
        local_sock.async_connect(ep, BIND_FN1(on_connect, _1));
    }
}

void ClientHandler::do_write()
{
    if (stopping)
        return;

    if (stage == STAGE_INIT)
    {
        gen_init_package(1, uid);
    }
    else if (stage == STAGE_CRYPT)
    {
        gen_crypt_negotiation();
    }
    if (!write_buff.size())
    {
        LOG_DEBUG(log) << ("Noting to write");

    }
    asio::async_write(local_sock, write_buff.data(), BIND_FN2(on_write, _1, _2));
}

void ClientHandler::do_read()
{
    LOG_NAME("do_read");
    if (stopping)
        return;
    switch (stage)
    {
    case STAGE_INIT:

        break;
    default:
        LOG_ERROR(log) << "Unknown stage status";
        stop();
    }
    asio::async_read_until(local_sock, read_buff, '\n', BIND_FN2(on_read, _1, _2));
}

void ClientHandler::on_read(const b_error_code &ec, size_t size)
{
    if (check_err(ec))
        return;

    if (stage == STAGE_INIT)
    {
        if (!stage_init_read())
        {
            stop();
        }
        else {
            LOG_NOTICE(log) << "change status to crypt";
            stage = STAGE_CRYPT;
        }
       
    }
    do_write();
}

void ClientHandler::on_write(const b_error_code &ec, size_t size)
{
    if (ec)
    {
        LOG_ERROR(log) << "Error occured in on_write" << ec.message();
        stop();
    }
    LOG_DEBUG(log) << "successfully write " << size << " bytes" << &write_buff;
    // write_buff.consume(size);
    if (stage == STAGE_INIT)
    {
        do_read();
    }
}

void ClientHandler::on_connect(const b_error_code &err)
{
    if (err)
    {
        LOG_ERROR(log) << "Error occurred in connect " << err.message();
        stop();
        return;
    }
    LOG_NOTICE(log) << "Connected successfully to " << peer_ep.address().to_string();
    if (stage == STAGE_INIT)
    {
        do_write();
    }
}

void ClientHandler::start(string &_uid)
{
    uid = _uid;
    do_connect(peer_ep);
    run();
}

size_t ClientHandler::read_completion(const b_error_code &err, size_t bytes)
{
    if (err)
    {
        LOG_ERROR(log) << "read completion: " << err.message();
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

bool ClientHandler::stage_init_read()
{
    istream in(&read_buff);
    char version, cmd;
    string b64str;
    in >> b64str;
    string res;

    b64_decode(&res, b64str);

    version = res[0];
    cmd = res[1];

    if (!check_version(version))
        return false;
    if (cmd == 0x11)
    { // ok
        return true;
    }
    else if (cmd == 0x12)
    { // uid is incorrect
        LOG_ERROR(log) << "from server: UID is incorrect";
    }
    else
        LOG_ERROR(log) << "Unknown cmd field in stage init";
    return false;
}

bool ClientHandler::gen_crypt_negotiation(char ver)
{
   // write_buff
    if (client->sym_key == nullptr) // for first call
    {
        size_t n_len, e_len;
        client->crypto->get_raw_public_key(NULL, &n_len, NULL, &e_len);
        uint8_t n[n_len], e[e_len];
        client->crypto->get_raw_public_key(n, &n_len, e, &e_len);

        ostream out(&write_buff);
        out.clear();
        stringstream ss;
        char mode = 0x81;
        uint8_t t = mode + 1;
        ss << ver << (char)1 << mode << (char)(n_len * 8 / 1024) << n << e;
        char * tmp = (char*)ss.str().c_str();
        string res;
        b64_encode(&res, ss.str());
        LOG_DEBUG(log) << "gen_crypt: Send data: " << res;
        out << res << '\n';
    }
}
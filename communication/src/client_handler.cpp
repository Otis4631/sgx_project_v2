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
    else if(stage == STAGE_UPLOAD) {
        print_bar(3);
        do_stop("stopping at upload stage");
        return ;
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
    case STAGE_CRYPT:
        break;
    default:
        LOG_ERROR(log) << "Unknown stage status";
        stop();
    }
    read_buff.consume(read_buff.size());

    asio::async_read_until(local_sock, read_buff, '\n', BIND_FN2(on_read, _1, _2));

    // asio::async_read(local_sock, read_buff.prepare(READ_BUFF_SIZE), 
    //                         BIND_FN2(on_read, _1, _2)); // TODO: No read limit}
}


void ClientHandler::on_read(const b_error_code &ec, size_t size)
{
    if (check_err(ec))
        return;
    read_buff.commit(size);
    if (stage == STAGE_INIT)
    {
        if (!stage_init_read())
        {
            stop();
        }
        else
        {
            LOG_NOTICE(log) << "change status to crypt";
            if(client->cmd == "verify") {
               // stage = VERIFY;
            }
            stage = STAGE_CRYPT;
        }
    }
    else if(stage == STAGE_CRYPT) {
        if (!handle_crypt_read())
        {
            stop();
        }
        else
        {
            LOG_NOTICE(log) << "change status to upload";
            stage = STAGE_UPLOAD;
        }
    }
    else do_stop("unknown stage status");
    do_write();
}

void ClientHandler::on_write(const b_error_code &ec, size_t size)
{
    LOG_NAME("on_write");
    if (ec)
    {
        LOG_ERROR(log) << "Error occured in on_write" << ec.message();
        stop();
    }
    LOG_DEBUG(log) << "successfully write " << size << " bytes";
    write_buff.consume(size);
    if (stage == STAGE_INIT)
    {
        // stage = STAGE_CRYPT;
        // LOG_DEBUG(log) << ("chage stage to crypt");
    }
    else if(stage == STAGE_CRYPT) {}
    else {
        LOG_DEBUG(log) << ("Unkown stage!");
        stop();
    }
    do_read();
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
    LOG_NAME("ClientHandler");
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
        char mode = 0x11; // TODO: 支持可选其他加密方式，现在是 AES_128_GCM, RSA_OAEP
        size_t data_len = CRYPT_HEADER_SIZE + n_len + e_len;
        uint8_t tmp[data_len];
        tmp[0] = ver;
        tmp[1] = 1;
        tmp[2] = mode;
        tmp[3] =  (char)(n_len * 8 / 1024);
        memcpy(tmp + 4, n, n_len);
        memcpy(tmp + 4 + n_len, e, e_len);

        string res;
        string s(tmp, tmp + data_len);
        b64_encode(&res, s);
        // LOG_DEBUG(log) << "gen_crypt: Send data: " << res;
        out << res << '\n';
    }
}

bool ClientHandler::handle_crypt_read() {
    istream in(&read_buff);
    char ver, mode;
    size_t key_len, padding_size=0;
    string b64data;
    in >> b64data;
    padding_size = count(b64data.begin(), b64data.end(), '=');
    string res;
    b64_decode(&res, b64data);
    while(padding_size --)
        res.pop_back();
    
    size_t data_size = res.size();
    ver = res[0];
    _cmd = res[1];
    mode = res[2];
    key_len = res[3] * 128;
    string data(res.begin() + 4, res.end());

    client->crypto->set_mode(RSA_OAEP | MODE_DECRYPT);
    client->crypto->open_private_key(client->sk_path.c_str(), nullptr);

    uint8_t* p_key = new uint8_t[16]; // TODO: support other cipher options
    uint8_t* iv = new uint8_t[12];
    size_t out_len = 16;
    printf("key is:\n");
    client->crypto->crypt(p_key, &out_len, (uint8_t*)data.c_str(), key_len);
    print_string2hex(p_key, 16);
    client->crypto->set_mode(AES_128_GCM | MODE_ENCRYPT);
    if(client->crypto->AES_init(p_key, iv) < 0) {
        LOG_ERROR(log) << "failed to init aes cipher";
        return false;
    }
    return true;
}
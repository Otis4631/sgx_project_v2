
#include "server_all.h"

void ServerHandler::start()
{
    LOG_DEBUG(log) << "Got Connection from" << local_sock.remote_endpoint();
    last_ping = boost::posix_time::microsec_clock::local_time();
    do_read();
    run();
}

void ServerHandler::on_write(const b_error_code &ec, size_t size)
{
    if (ec)
    {
        do_stop(ec.message());
        return;
    }
    if (stage == STAGE_INIT)
    {
        // auto cb = write_buff.data();
        // string b64( boost::asio::buffers_begin(cb), boost::asio::buffers_end(cb));
        // string res;
        // b64_decode();
        if ((_cmd) == 0x11) // ok
        {
            stage = STAGE_CRYPT;
            LOG_DEBUG(log) << "on_write: change status to crypt";
        }
        else if ((_cmd) == 0x12)
        {
            stage = STAGE_DESTORYED;
            LOG_DEBUG(log) << "on_write: change status to destoryed";
        }
    }
    else if (stage == STAGE_CRYPT)
    {
        stage = STAGE_DOWNLOAD;
        LOG_DEBUG(log) << "on_write: change status to download";
        LOG_DEBUG(log) << "successfully write " << size << " bytes";
    }
    write_buff.consume(size);
    do_read();
}

void ServerHandler::do_read()
{
    if (stopping)
        return;
    if (stage == STAGE_INIT)
    {
    }
    else if (stage == STAGE_CRYPT)
    {
    }
    else if(stage == STAGE_DOWNLOAD) {
        do_stop("stopping at DOWNLOAD status");
        return ;
    }
    read_buff.consume(read_buff.size());
    asio::async_read(local_sock, read_buff.prepare(READ_BUFF_SIZE), 
                            boost::bind(&Handler::read_completion, shared_from_this(), _1, _2),
                            BIND_FN2(on_read, _1, _2)); // TODO: No read limit
}

void ServerHandler::on_read(const b_error_code &ec, size_t size)
{
    LOG_NAME("Server_Read");
    if (ec)
    {
        LOG_ERROR(log) << "Error occurred " << ec.message();
        stop();
    }
    read_buff.commit(size);
    const uint8_t *tmp = (const uint8_t *)boost::asio::buffer_cast<const void *>(read_buff.data());


    if (stage == STAGE_INIT)
    {
        if (!handle_init_read())
        {
            do_stop("Unknown command type in stage init");
        }
    }
    else if (stage == STAGE_CRYPT)
    {

        if (!handle_crypt_read())
        {
            stop();
        }
    }
    else {
        LOG_ERROR(log) << "Unknow stage status";
        stop();
    }
//    read_buff.consume(size);
    do_write();
}

void ServerHandler::do_write()
{
    if (stopping)
        return;
    if (stage == STAGE_INIT)
    {
    }
    else if (stage == STAGE_CRYPT)
    {
    }
    asio::async_write(local_sock, write_buff.data(), BIND_FN2(on_write, _1, _2));
}

bool ServerHandler::auth_uid(string &s)
{
    LOG_DEBUG(log) << "auth_id: " << s;
    if (s == "lizheng")
    {
        LOG_NOTICE(log) << "uid is correct";
        return true;
    };
    LOG_NOTICE(log) << "uid is incorrect";
    return false;
}


bool ServerHandler::handle_init_read()
{
    istream in(&read_buff);
    char ver, cmd;
    string read_data;
    string out;
    in >> read_data;
    b64_decode(&out, read_data);
    cmd = out[1];
    string _uid = out.substr(2, out.size() - 2);

    if (cmd == 0x01)
    {
        if (!auth_uid(_uid))
        {
            gen_init_package(0x12, ""); // uid is incorrect
        }
        gen_init_package(0x11, ""); // uid is ok
    }
    else
    {
        return false;
    }
    return true;
}

bool ServerHandler::handle_crypt_read()
{
    LOG_NAME("Crypt_Handler");
    if (read_buff.size() < CRYPT_HEADER_SIZE)
    {
        LOG_ERROR(log) << "crpyt data size " << read_buff.size() << " is less than header size";
        return false;
    }
    istream in(&read_buff);
    char ver, cmd, mode, n_len_bin;
    size_t n_len;
    size_t data_len;
    size_t e_len;
    size_t iv_len;
    int padding_size = 0;
    string b64data, data, tmp;
    in >> b64data;
    padding_size = count(b64data.begin(), b64data.end(), '=');
    b64_decode(&tmp, b64data);
    ver = tmp[0];
    cmd = tmp[1];
    mode = tmp[2];
    n_len_bin = tmp[3];

    data = string(tmp.begin() + 4, tmp.end());

    if (!check_version(ver))
    {
        return false;
    }

    data_len = data.size() - padding_size;
    n_len = ((size_t)n_len_bin * 1024) / 8;
    if (data_len <= n_len)
    {
        LOG_ERROR(log) << "data size " << data_len << " is less than or equal to n size";
        return false;
    }
    e_len = data_len - n_len;
    classifier = make_shared<Classifier>();
    uint8_t *n = new uint8_t[n_len], *e = new uint8_t[e_len];
    memcpy(n, data.c_str(), n_len);
    memcpy(e, data.c_str() + n_len, e_len);
    if ((mode & 0x0f) == 0x01)
    { // AES_128_GCM

        iv_len = 12;
    }
    else
    {
        LOG_ERROR(log) << "Unsported cipher type";
        stop();
        return false;
    }

    uint8_t *key = new uint8_t[n_len];
    uint8_t *iv = new uint8_t[iv_len];
    classifier->init_crypto(n, n_len, e, e_len, key, iv, iv_len);

    string key_s(key, key + n_len);
    string iv_s(iv, iv + iv_len);

    delete[] key;
    delete[] iv;

    return gen_crypt_response(key_s, iv_s);
}

bool ServerHandler::gen_crypt_response(string &key, string &iv)
{
    ostream out(&write_buff);
    stringstream ss;
    _cmd = 0x11;
    char key_size_bin = key.size() / 128; 
    ss << VERSION << _cmd << (char)0x11 << key_size_bin << key << iv;
    string res;
    b64_encode(&res, ss.str());
   // LOG_DEBUG(log) << read;
    out << res << '\n';
    return true;
}
#pragma once

// #include "client_all.h"  can not be load
#include "common.h"
#include "batch_data.hpp"
#include "client_handler.h"
#include "openssl_crypto.h"

class ClientHandler;
// using namespace boost;
class Client : public std::enable_shared_from_this<Client>
{
public:
    Client(ptree _config) : config(_config)
    {
        LOG_NAME("Client");

        uid = config.get<string>("common.uid");
        string data_config_path = config.get<string>("data.data_config_path");
        ptree data_config;
        if (parse_config(data_config_path, data_config) != 0)
        {
            LOG_ERROR(lg) << "failed to parse config file " << data_config_path;
            exit(-1);
        }
        size_t batch = data_config.get<size_t>("batch");
        string data_path = data_config.get<string>("data_path");
        size_t w = data_config.get<size_t>("width");
        size_t h = data_config.get<size_t>("height");
        size_t c = data_config.get<size_t>("channel");
        size_t img_size = w * h * c;
        size_t total = data_config.get<size_t>("size");
        data = make_shared<BatchData>(data_path, to_datatype(data_config.get<string>("type")),
                                      batch, img_size, total);

        crypto = make_shared<OpenSSLCrypto>(RSA_OAEP);

        string pk_path = config.get<string>("common.pk_path");
        string sk_path = config.get<string>("common.sk_path");
        crypto->open_public_key(pk_path.c_str());
    };
    void run();

    ptree config;
    shared_ptr<ClientHandler> handler;
    string uid;
    shared_ptr<BatchData> data;
    shared_ptr<OpenSSLCrypto> crypto;

    shared_ptr<char> sym_key = nullptr;
    shared_ptr<char> sym_key_id = nullptr;

    log_t lg;
};

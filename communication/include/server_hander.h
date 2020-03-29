#pragma once
#include "server_all.h"

typedef class ServerHandler self_type;

class ServerHandler: public Handler, public std::enable_shared_from_this<ServerHandler>
{
    public:
        ServerHandler()
            : Handler(), stage(STAGE_INIT)
        {
        }
        void do_read();
        void on_read(const b_error_code &ec, size_t size);

        void do_write();
        void on_write(const b_error_code &ec, size_t size);

        void init(string &_uid);
        bool auth_uid(string &s);
        void start();

        void do_stop(string err_message);

        size_t read_completion(const b_error_code &err, size_t bytes);

        ~ServerHandler()
        {
            if (!stopping)
                stop();
        }

    private:
        string uid;
        int stage;
};

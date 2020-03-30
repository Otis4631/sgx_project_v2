#pragma once
#include <stdint.h>
#include <string>
#include <boost/asio.hpp>
#include "common.h"

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

using namespace std;
using namespace boost::archive::iterators;

namespace asio = boost::asio;
typedef boost::system::error_code b_error_code;

typedef boost::shared_ptr<asio::ip::tcp::socket> p_socket_t;

typedef std::shared_ptr<asio::deadline_timer> p_timer_t;

class Data
{
};

typedef struct tag_timer
{
    tag_timer(p_timer_t &&t, string s) : timer(t), expire_times(0), expired(false), tag(s){};
    p_timer_t timer;
    int expire_times;
    bool expired;
    boost::posix_time::ptime last_expired;
    string tag;

} tag_timer_t;

typedef shared_ptr<tag_timer_t> p_tag_timer_t;

class Handler
{
public:
    Handler() : local_sock(service), stopping(false)
    {
        timer_init();
    };

    Handler(string &_addr, int &_port) : local_sock(service), stopping(false)
    {
        timer_init();
        asio::ip::tcp::resolver resolver(service);
        auto iter = resolver.resolve(asio::ip::tcp::resolver::query(_addr, to_string(_port)));
        peer_ep = *iter;
    }

    void timer_init()
    {
        timer_map["read"] = std::make_shared<tag_timer_t>(std::make_shared<asio::deadline_timer>(service), "read");
        timer_map["write"] = std::make_shared<tag_timer_t>(std::make_shared<asio::deadline_timer>(service), "write");
        timer_map["ping"] = std::make_shared<tag_timer_t>(std::make_shared<asio::deadline_timer>(service), "ping");
    }

    asio::ip::tcp::socket &get_sock()
    {
        return local_sock;
    }

    void gen_init_package(int8_t cmd, string data, int8_t verion = 1)
    {
        _cmd = cmd;
        ostream out(&write_buff);
        out.clear();
        stringstream ss;
        ss << verion << cmd << data;
        string res;
        b64_encode(&res, ss.str());
        LOG_DEBUG(log) << "Sending data: " << res;
        out << res << '\n';
    }
    void run()
    {
        if (stopping)
            return;
        LOG_NOTICE(log) << "Handler is running successfully";
        service.run();
    }

    void stop()
    {
        if (stopping)
            return;
        stopping = true;
        LOG_NOTICE(log) << "Stopping Handler...";
        local_sock.close();
    }

    bool check_err(const b_error_code &err)
    {
        if (!err)
            return false;
        LOG_ERROR(log) << err.message();
        stop();
        return true;
    }

    bool check_version(char version)
    {
        if (version < MIN_VERSION)
        {
            LOG_ERROR(log) << "client protocol is too old";
            stop();
            return false;
        }
        return true;
    }

    void do_start_timer(p_tag_timer_t &t, int sec)
    {
        auto timer = t->timer;
        timer->expires_from_now(boost::posix_time::seconds(sec));
        // timer.expires_from_now();
        timer->async_wait(boost::bind(&Handler::handle_wait, this, _1, boost::ref(t)));
    }

    void handle_wait(const b_error_code &err, p_tag_timer_t &t)
    {
        if (err)
        {
            if (err == boost::system::errc::operation_canceled)
            {
                LOG_DEBUG(log) << "timer " << t->tag << "is canceled";
            }
        }
        else
            LOG_DEBUG(log) << "timer " << t->tag << "is timeout";
    }

    void send_ping()
    {
        gen_init_package(0x1f, "");
    }

    bool b64_encode(string *outPut, const string &inPut)
    {
        typedef base64_from_binary<transform_width<string::const_iterator, 6, 8>> Base64EncodeIter;

        stringstream result;
        copy(Base64EncodeIter(inPut.begin()),
             Base64EncodeIter(inPut.end()),
             ostream_iterator<char>(result));

        size_t Num = (3 - inPut.length() % 3) % 3;
        for (size_t i = 0; i < Num; i++)
        {
            result.put('=');
        }
        *outPut = result.str();
        return outPut->empty() == false;
    }
    bool b64_decode(string *outPut, const string &inPut)
    {
        typedef transform_width<binary_from_base64<string::const_iterator>, 8, 6> Base64DecodeIter;

        stringstream result;
        try
        {
            copy(Base64DecodeIter(inPut.begin()),
                 Base64DecodeIter(inPut.end()),
                 ostream_iterator<char>(result));
        }
        catch (...)
        {
            return false;
        }
        *outPut = result.str();
        return outPut->empty() == false;
    }

protected:
    asio::io_service service;
    asio::ip::tcp::socket local_sock;
    asio::ip::tcp::endpoint peer_ep;
    map<string, p_tag_timer_t> timer_map;

    string addr;
    int port;
    bool stopping;
    char _cmd;
    asio::streambuf read_buff;
    asio::streambuf write_buff;

    src::severity_logger<severity_level> log;

    int stage;
    boost::posix_time::ptime last_ping;
};
